#pragma once
#include <vector>
#include <string>
#include <sys/types.h>
#include <algorithm>
#include <assert.h>


// һ��buffer����ͼ��ʾ(from chenshuo)
///
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size

///���У�readable bytesΪ�ɶ����ݳ��ȣ�writable bytesΪ��д���ݳ���
///��ʵ����bufferΪһ�����У���input bufferΪ�����ͻ��˴�������ݲ��ϵطŵ�writable�б�Ϊreadable��writerIndex�����
///���������벻�ϴ�readable�н��ж�ȡ��readerIndex�����

class Buffer{
public:
    static const size_t kCheapPrepend = 8;   //���ڼ�¼���ݰ��ĳ��ȣ����ճ������
    static const size_t kInitialSize = 1024; //Ĭ�ϻ���������


    /***
     *  @brief ���캯��������һ��buffer
     *  @param initialSize Buffer��ʼ��С��Ĭ��Ϊ1024
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
     *  @brief ����readable����С
    ***/
    void swap(Buffer& rhs){
        buffer_.swap(rhs.buffer_);
        std::swap(readerIndex_, rhs.readerIndex_);
        std::swap(writerIndex_, rhs.writerIndex_);
    }

    /***
     *  @brief ����readable����С
    ***/
    size_t readableBytes() const{ 
        return writerIndex_ - readerIndex_; 
    }

    /***
     *  @brief ����writable����С
    ***/
    size_t writableBytes() const{
        return buffer_.size() - writerIndex_; 
    }

    /***
     *  @brief ����prependable����С
    ***/
    size_t prependableBytes() const{
        return readerIndex_; 
    }

    /***
     *  @brief ����readable����ʼ��ַ
    ***/
    const char* peek() const{ 
        return begin() + readerIndex_; 
    }



    /***
     *  @brief �ӻ������л�ȡ����
     *  @param len ��ȡ���ݴ�С
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
     *  @brief �ӻ������л�ȡ����
     *  @param end ��ȡ���ݵ�ĩβָ��
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
     *  @brief ��string��ʽ�ӻ������л�ȡ��������
    ***/
    std::string retrieveAllAsString(){
        return retrieveAsString(readableBytes());
    }

    /***
     *  @brief ��string����ʽ�ӻ������л�ȡ����
     *  @param len ���ݳ���
    ***/
    std::string retrieveAsString(size_t len){
      assert(len <= readableBytes());
      std::string result(peek(), len);
      retrieve(len);
      return result;
    }

    /***
     *  @brief ��data��ָ����Ϊlen������д�뻺������
     *  @param data ����ָ��
     *  @param len ���ݳ���
    ***/
    void append(const char* data, size_t len){
      ensureWritableBytes(len);
      std::copy(data, data+len, beginWrite());
      hasWritten(len);
    }

    /***
     *  @brief ������д�뻺������
     *  @param data ����
     *  @param len ���ݳ���
    ***/
    void append(const void*  data, size_t len){
      append(static_cast<const char*>(data), len);
    }

    /***
     *  @brief �򻺳���д������֮ǰ����Ҫ�жϻ����������Ƿ��㹻д�볤��Ϊlen������
     *         �����������������ݣ�����vector��
     *  @param len ���ݴ�С
    ***/
    void ensureWritableBytes(size_t len){
      if (writableBytes() < len){
        makeSpace(len);
      }
      assert(writableBytes() >= len);
    }



    /***
     *  @brief ��ȡwriteable������ʼ��ַ
     *  @return ����writeable������ʼ��ַ
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
     *  @brief ���пͻ��˵����ݵ���ʱ���ú����ӽ��ջ������н����ݶ�ȡ����ŵ�Buffer��
     *  @param fd 
     *  @param savedErrno 
     *  @return 
    ***/
    ssize_t readFd(int fd, int* savedErrno);


    /***
     *  @brief ������ͻ��˷�������ʱ���ú����ɽ����ݴ�Buffer���������ͻ�������
     *  @param fd 
     *  @param savedErrno 
     *  @return 
    ***/
    ssize_t writeFd(int fd, int* savedErrno);

private:

    char* begin(){ return &*buffer_.begin(); }

    const char* begin() const{ return &*buffer_.begin(); }


    /***
     *  @brief ��buffer�������ݣ����buffer�����Ѿ��ͷŵĿռ䲢�㹻ʹ�ã�����������
     *  @param len ���ݴ�С
    ***/
    void makeSpace(size_t len){
        if (writableBytes() + prependableBytes() >= len + kCheapPrepend){
            //buffer��Ϊ����ȡ���ͷŵĿռ�prependable�㹻len��С������ʹ�ã���������
            size_t readable = readableBytes();
            std::copy(begin()+readerIndex_, begin()+writerIndex_, begin()+kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
        else{
            //bufferʵ��û����ռ���
            buffer_.resize(writerIndex_+len);
        }
    }

private:
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

    static const char kCRLF[];
};