/*
 * @Author: lvxr
 * @Date: 2024-03-03 15:02:02
 * @LastEditTime: 2024-03-04 21:30:41
 */
#include "EventLoop.h"

#include <sys/eventfd.h>
#include <memory>
#include <errno.h>

#include "Poller.h"
#include "Channel.h"
#include "Logger.h"
#include "CurrentThread.h"

//__thread是一个thread_local的机制，代表这个变量是这个线程独有的全局变量，而不是所有线程共有
__thread EventLoop *t_loopInThisThread = nullptr; // 防止一个线程创建多个EventLoop
// 当一个eventloop被创建起来的时候,这个t_loopInThisThread就会指向这个Eventloop对象
// 如果这个线程又想创建一个EventLoop对象的话这个t_loopInThisThread非空，就不会再创建了

const int kPollTimeMs = 10000; // 定义默认的Pooler IO复用接口的超时时间

// 创建wakeupfd，用来通notify subreactor处理新来的channelx
int createEventfd()
{
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
        LOG_FATAL("eventfd error: %d \n", errno);
    return evtfd;
}

EventLoop::EventLoop() : looping_(false),
                         quit_(false),
                         callingPendingFunctors_(false),
                         threadId_(CurrentThread::tid()),              // 获取当前线程的tid
                         poller_(Poller::newDefaultPoller(this)),      // 获取一个封装着控制epoll操作的对象
                         wakeupFd_(createEventfd()),                   // 生成一个eventfd，每个EventLoop对象，都会有自己的eventfd
                         wakeupChannel_(new Channel(this, wakeupFd_)), // 每个channel都要知道自己所属的eventloop
                         currentActiveChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loopInThisThread) // 如果当前线程已经绑定了某个EventLoop对象了，那么该线程就无法创建新的EventLoop对象了
        LOG_FATAL("Another EventLoop %p exits in this thread %d \n", t_loopInThisThread, threadId_);
    else
        t_loopInThisThread = this;
    // 一开始wakeupChannel_并不在事件循环中，在设置回调函数后才被事件循环所监听
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading(); // 每一个EventLoop都将监听wakeupChannel的EpollIN读事件了。
    // mainReactor通过给wakeupFd_给sbureactor写东西。
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    close(wakeupFd_); // 回收资源
    t_loopInThisThread = nullptr;
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof(one)); // mainReactor给subreactor发消息，subReactor通过wakeupFd_感知。
    if (n != sizeof(one))
        LOG_ERROR("EventLoop::handleRead() reads %d bytes instead of 8", n);
}

void EventLoop::loop()
{
    // EventLoop 所属线程执行
    looping_ = true;
    quit_ = false;
    LOG_INFO("EventLoop %p start looping \n", this);
    while (!quit_)
    {
        // 清空当前活跃数组
        activeChannels_.clear();

        // 监听两类fd，一个是client的fd，一个是wakeupfd，用于mainloop和subloop的通信
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_); // 此时activeChannels已经填好了事件发生的channel
        for (Channel *channel : activeChannels_)
        {
            // Poller监听哪些channel发生事件了，然后上报（通过activeChannels）给EventLoop，通知channel处理相应事件
            channel->HandlerEvent(pollReturnTime_);
        }

        doPendingFunctors(); // 执行当前EventLoop事件循环需要处理的回调操作。
    }
    LOG_INFO("EventLoop %p stop looping. \n", t_loopInThisThread);
}

void EventLoop::quit()
{
    // 退出事件循环，有可能是loop在自己的线程中调用quit，loop()函数的while循环发现quit_=true就结束了循环
    // 如果是在其他线程中调用的quit，在一个subloop线程中调用了mainloop的quit，那就调用wakeup唤醒mainLoop线程
    quit_ = true;
    if (!isInLoopThread())
        wakeup();
}

void EventLoop::runInLoop(Functor cb)
{
    // 保证了调用这个cb一定是在其EventLoop线程中被调用。
    if (isInLoopThread())
        // 如果当前调用runInLoop的线程正好是EventLoop的运行线程，则直接执行此函数
        cb();
    else
        // 否则调用 queueInLoop 函数
        queueInLoop(cb);
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    // 唤醒相应的，需要执行上面回调操作的loop线程
    // || callingPendingFunctors_的意思是：当前loop正在执行回调，但是loop又有了新的回调
    //  这个时候就要wakeup()loop所在线程，让它继续去执行它的回调。
    if (!isInLoopThread() || callingPendingFunctors_)
        wakeup();
}

void EventLoop::wakeup()
{
    // 想wakeupFd_中写，触发写事件，马上执行doPendingFunctors
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(n))
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
}

void EventLoop::updateChannel(Channel *channel) { poller_->updateChannel(channel); }
void EventLoop::removeChannel(Channel *channel) { poller_->removeChannel(channel); }
bool EventLoop::hasChannel(Channel *channel) { poller_->hasChannel(channel); }

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // 这里交换避免频繁触发锁，更效率
        functors.swap(pendingFunctors_); // 这里的swap其实只是交换的vector对象指向的内存空间的指针而已。
    }
    for (const Functor &functor : functors)
        functor();
    callingPendingFunctors_ = false;
}