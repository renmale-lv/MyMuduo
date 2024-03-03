/*
 * @Author: lvxr
 * @Date: 2024-03-02 19:33:43
 * @LastEditTime: 2024-03-02 20:30:14
 */

#ifndef BUFFER_H
#define BUFFER_H

/*
Buffer
+-------------------------+----------------------+---------------------+
|    prependable bytes    |    readable bytes    |    writable bytes   |
|                         |      (CONTENT)       |                     |
+-------------------------+----------------------+---------------------+
|                         |                      |                     |
0        <=           readerIndex     <=     writerIndex             size

readable bytes空间才是要服务端要发送的数据，writable bytes空间是从socket读来的数据存放的地方。
readable可读的数据，即可被从缓冲区读出发送的数据
prependable预留空间，用于记录数据的长度
*/

#include <vector>
#include <sys/types.h>
#include <string>
#include <sys/uio.h>
#include <errno.h>
#include <unistd.h>

// Buffer缓冲区类
class Buffer
{
public:
    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend) {}

    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }
    size_t prependableBytes() const { return readerIndex_; }

    /**
     * @description: 返回可读数据的起始地址
     */
    const char *peek() const { return begin() + readerIndex_; }

    /**
     * @description: 从头开始清空数据，用于进行复位操作
     * @param {size_t} len 要清空数据的长度
     */
    void retrieve(size_t len)
    {
        if (len < readableBytes())
            readerIndex_ += len; // 应用只读取可读缓冲区数据的一部分，就是len
        else
            retrieveAll();
    }

    /**
     * @description: 清空缓冲区
     */
    void retrieveAll() { readerIndex_ = writerIndex_ = kCheapPrepend; }

    /**
     * @description: 读出全部可读数据，并返回字符串
     */
    std::string retrieveAllString() { return retrieveAsString(readableBytes()); }

    /**
     * @description: 从头开始获取数据，并以string形式返回
     * @param {size_t} len 要获得的数据长度
     */
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    /**
     * @description: 确保缓冲区可以写入数据，空间不够则进行扩容
     * @param {size_t} len 要写入数据的长度
     */
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
            makeSpace(len);
    }

    /**
     * @description: 往缓冲区里写入数据
     * @param {char} *data 要写入的数据
     * @param {size_t} len 要写入数据的长度
     */
    void append(const char *data, size_t len)
    {
        // 先确保空间是否足够
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    char *beginWrite() { return begin() + writerIndex_; }
    const char *beginWrite() const { return begin() + writerIndex_; }

    /**
     * @description: 从客户端套接字fd上读取数据
     * @param {int} fd 客户端套接字
     * @param {int} *saveErrno 函数运行时产生的错误
     */
    ssize_t readFd(int fd, int *saveErrno)
    {
        char extrabuf[65536] = {0}; // 栈上的内存空间
        struct iovec vec[2];
        const size_t writableSpace = writableBytes(); // 可写缓冲区的大小
        vec[0].iov_base = begin() + writerIndex_;     // 第一块缓冲区
        vec[0].iov_len = writableSpace;               // 当我们用readv从socket缓冲区读数据，首先会先填满这个vec[0]
                                                      // 也就是我们的Buffer缓冲区
        vec[1].iov_base = extrabuf;                   // 第二块缓冲区，如果Buffer缓冲区都填满了，那就填到我们临时创建的
        vec[1].iov_len = sizeof(extrabuf);            // 栈空间上。
        const int iovcnt = (writableSpace < sizeof(extrabuf) ? 2 : 1);
        // 如果Buffer缓冲区大小比extrabuf(64k)还小，那就Buffer和extrabuf都用上
        // 如果Buffer缓冲区大小比64k还大或等于，那么就只用Buffer。这意味着，我们最少也能一次从socket fd读64k空间

        // readv集中读
        const ssize_t n = ::readv(fd, vec, iovcnt);
        if (n < 0)
        {
            *saveErrno = errno; // 出错了！！
        }
        // Buffer空间足够
        else if (n <= writableSpace)
        {
            writerIndex_ += n;
        }
        // Buffer空间不够存，需要把溢出的部分（extrabuf）倒到Buffer中（会先触发扩容机制）
        else
        {
            writerIndex_ = buffer_.size();
            append(extrabuf, n - writableSpace);
        }
        return n;
    }

    /**
     * @description: 向socket fd上写数据
     * @param {int} *saveErrno 函数运行时产生的错误
     */
    ssize_t writeFd(int fd, int *saveErrno)
    {
        const size_t readableSpace = readableBytes();
        ssize_t n = ::write(fd, peek(), readableSpace); // 从Buffer中有的数据(readableBytes)写到socket中
        if (n < 0)
            *saveErrno = errno;
        return n;
    }

private:
    /*
     * buffer_.begin()返回一个迭代器，指向容器的第一个元素
     * *buffer_.begin()解引用该迭代器，得到第一个元素的值
     * &*buffer_.begin()取得该值的地址，即获取第一个元素的指针
     */
    char *begin() { return &*buffer_.begin(); }
    const char *begin() const { return &*buffer_.begin(); }

    /**
     * @description: 扩容，确保可以写入数据
     * @param {size_t} len 要写入数据的长度
     */
    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() - kCheapPrepend < len)
        {
            // 能用来写的缓冲区大小 < 我要写入的大小len，那么就要扩容了
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            // 如果能写的缓冲区大小 >= 要写的len，那么说明要重新调整一下Buffer的两个游标了。
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,   // 源 首迭代器
                      begin() + writerIndex_,   // 源 尾迭代器
                      begin() + kCheapPrepend); // 目标 首迭代器
            // 这里把可读区域的数据给前移了
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

public:
    // static const int可以在类里面初始化，是因为它既然是const的，那程序就不会再去试图初始化了
    static const size_t kCheapPrepend = 8;   // 记录数据包的长度的变量长度，用于解决粘包问题
    static const size_t kInitialSize = 1024; // 缓冲区长度

private:
    // 为什么要用vector，因为可以动态扩容
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};

#endif