#pragma once
#include <vector>
#include <string>
#include <sys/types.h>
#include <algorithm>
#include <assert.h>


// 一个buffer如下图所示(from chenshuo)
///
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size

///其中，readable bytes为可读数据长度，writable bytes为可写数据长度
///即实际上buffer为一个队列，以input buffer为例，客户端传入的数据不断地放到writable中变为readable，writerIndex向后移
///服务器代码不断从readable中进行读取，readerIndex向后移

class Buffer{
public:
    static const size_t kCheapPrepend = 8;   //用于记录数据包的长度，解决粘包问题
    static const size_t kInitialSize = 1024; //默认缓冲区长度


    /***
     *  @brief 构造函数，创建一个buffer
     *  @param initialSize Buffer初始大小，默认为1024
    ***/
    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend){

        assert(readableBytes() == 0);
        assert(writableBytes() == initialSize);
        assert(prependableBytes() == kCheapPrepend);
    }

    /***
     *  @brief 返回readable区大小
    ***/
    void swap(Buffer& rhs){
        buffer_.swap(rhs.buffer_);
        std::swap(readerIndex_, rhs.readerIndex_);
        std::swap(writerIndex_, rhs.writerIndex_);
    }

    /***
     *  @brief 返回readable区大小
    ***/
    size_t readableBytes() const{ 
        return writerIndex_ - readerIndex_; 
    }

    /***
     *  @brief 返回writable区大小
    ***/
    size_t writableBytes() const{
        return buffer_.size() - writerIndex_; 
    }

    /***
     *  @brief 返回prependable区大小
    ***/
    size_t prependableBytes() const{
        return readerIndex_; 
    }

    /***
     *  @brief 返回readable区起始地址
    ***/
    const char* peek() const{ 
        return begin() + readerIndex_; 
    }



    /***
     *  @brief 从缓冲区中获取数据
     *  @param len 获取数据大小
    ***/
    void retrieve(size_t len){
        assert(len <= readableBytes());
        if (len < readableBytes()){
            readerIndex_ += len;
        }
        else{
            retrieveAll();
        }
    }

    /***
     *  @brief 从缓冲区中获取数据
     *  @param end 获取数据的末尾指针
    ***/
    void retrieveUntil(const char* end){
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }


    void retrieveAll(){
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    } 
    
    /***
     *  @brief 以string形式从缓冲区中获取所有数据
    ***/
    std::string retrieveAllAsString(){
        return retrieveAsString(readableBytes());
    }

    /***
     *  @brief 以string的形式从缓冲区中获取数据
     *  @param len 数据长度
    ***/
    std::string retrieveAsString(size_t len){
      assert(len <= readableBytes());
      std::string result(peek(), len);
      retrieve(len);
      return result;
    }

    /***
     *  @brief 将data所指长度为len的数据写入缓冲区中
     *  @param data 数据指针
     *  @param len 数据长度
    ***/
    void append(const char* data, size_t len){
      ensureWritableBytes(len);
      std::copy(data, data+len, beginWrite());
      hasWritten(len);
    }

    /***
     *  @brief 将数据写入缓冲区中
     *  @param data 数据
     *  @param len 数据长度
    ***/
    void append(const void*  data, size_t len){
      append(static_cast<const char*>(data), len);
    }

    /***
     *  @brief 向缓冲区写入数据之前，需要判断缓冲区容量是否足够写入长度为len的数据
     *         如果不够，则进行扩容（类似vector）
     *  @param len 数据大小
    ***/
    void ensureWritableBytes(size_t len){
      if (writableBytes() < len){
        makeSpace(len);
      }
      assert(writableBytes() >= len);
    }



    /***
     *  @brief 获取writeable区的起始地址
     *  @return 返回writeable区的起始地址
    ***/
    const char* beginWrite() const{
        return begin() + writerIndex_; 
    }


    void hasWritten(size_t len){
        assert(len <= writableBytes());
        writerIndex_ += len;
    }

    void unwrite(size_t len){
        assert(len <= readableBytes());
        writerIndex_ -= len;
    }


    void prepend(const void* /*restrict*/ data, size_t len){
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char* d = static_cast<const char*>(data);
        std::copy(d, d+len, begin()+readerIndex_);
    }


    /***
     *  @brief 当有客户端的数据到达时，该函数从接收缓冲区中将数据读取并存放到Buffer中
     *  @param fd 
     *  @param savedErrno 
     *  @return 
    ***/
    ssize_t readFd(int fd, int* savedErrno);


    /***
     *  @brief 当想向客户端发送数据时，该函数可将数据从Buffer拷贝到发送缓冲区中
     *  @param fd 
     *  @param savedErrno 
     *  @return 
    ***/
    ssize_t writeFd(int fd, int* savedErrno);

private:

    char* begin(){ return &*buffer_.begin(); }

    const char* begin() const{ return &*buffer_.begin(); }


    /***
     *  @brief 对buffer进行扩容，如果buffer中有已经释放的空间并足够使用，则无需扩容
     *  @param len 扩容大小
    ***/
    void makeSpace(size_t len){
        if (writableBytes() + prependableBytes() >= len + kCheapPrepend){
            //buffer因为被读取而释放的空间prependable足够len大小的数据使用，无需扩容
            size_t readable = readableBytes();
            std::copy(begin()+readerIndex_, begin()+writerIndex_, begin()+kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
        else{
            //buffer实在没多余空间了
            buffer_.resize(writerIndex_+len);
        }
    }

private:
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

    static const char kCRLF[];
};