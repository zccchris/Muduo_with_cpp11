#include "Buffer.h"


#include <errno.h>
#include <sys/uio.h>



const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;


ssize_t Buffer::readFd(int fd, int* savedErrno){
//该函数从客户端socketfd上读取数据，存入buffer中
//如果buffer容量足够，直接写入buffer中，否则写道extrabuf中
    char extrabuf[65536]; //栈上的内存空间
    struct iovec vec[2];  //io向量，其中的iov_base指向一个缓冲区，iov_len存放的是数据的最大长度，即缓冲区最大可用长度
    const size_t writable = writableBytes();
    vec[0].iov_base = begin()+writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    //如果buffer可用空间小于栈空间，则先用buffer，再用栈，两者都用上，iovcnt为2
    //如果buffer可用空间大于栈空间，则只用buffer。
    //这样，一次至少能从socketfd中读取64kB到缓冲区中
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0){
        *savedErrno = errno;
    }
    //读入的数据小于writable，说明buffer足够放了
    else if (static_cast<size_t>(n) <= writable){
        writerIndex_ += n;
    }
    //否则，writerIndex肯定到头了，并且，还要往里写入暂存在栈中的数据
    else{
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}