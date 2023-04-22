#include "Buffer.h"


#include <errno.h>
#include <sys/uio.h>



const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;


ssize_t Buffer::readFd(int fd, int* savedErrno){
//�ú����ӿͻ���socketfd�϶�ȡ���ݣ�����buffer��
//���buffer�����㹻��ֱ��д��buffer�У�����д��extrabuf��
    char extrabuf[65536]; //ջ�ϵ��ڴ�ռ�
    struct iovec vec[2];  //io���������е�iov_baseָ��һ����������iov_len��ŵ������ݵ���󳤶ȣ��������������ó���
    const size_t writable = writableBytes();
    vec[0].iov_base = begin()+writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    //���buffer���ÿռ�С��ջ�ռ䣬������buffer������ջ�����߶����ϣ�iovcntΪ2
    //���buffer���ÿռ����ջ�ռ䣬��ֻ��buffer��
    //������һ�������ܴ�socketfd�ж�ȡ64kB����������
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0){
        *savedErrno = errno;
    }
    //���������С��writable��˵��buffer�㹻����
    else if (static_cast<size_t>(n) <= writable){
        writerIndex_ += n;
    }
    //����writerIndex�϶���ͷ�ˣ����ң���Ҫ����д���ݴ���ջ�е�����
    else{
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}