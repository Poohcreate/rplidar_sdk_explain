/*
 *  RoboPeak Project
 *  HAL Layer - Socket Interface
 *  Copyright 2009 - 2013 RoboPeak Project
 *
 *  POXIS Implementation
 */


#include "sdkcommon.h"
#include "../../hal/socket.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

namespace rp{ namespace net {

/*
"static inline" 是一种函数修饰符，用于在代码中声明静态内联函数。
静态修饰符 "static" 用于在函数声明中指示它是一个静态函数。静态函数属于类的整体，不依赖于类的任何对象，因此不需要实例化即可调用。静态函数只能访问类的静态成员变量和静态函数，不能直接访问非静态成员变量和非静态函数。
内联修饰符 "inline" 用于在函数声明中指示它是一个内联函数。内联函数在编译时会直接将函数体的代码插入到调用处，而不会生成函数的调用与返回的开销。内联函数通常用于短小且频繁调用的函数，以提高代码执行效率。
静态内联函数的作用是将函数内联展开，并且在整个程序中只有一份函数的实例化。这可以减少函数调用的开销，并且可以省略额外的函数调用开销和内存占用。同时，静态内联函数只能在当前文件中使用，不能被其他文件访问。
总结起来，"static inline" 修饰的函数是一个静态内联函数，它在编译时被直接插入调用处，在整个程序中只有一份实例化，并且只能在当前文件中使用。
*/
static inline int _halAddrTypeToOSType(SocketAddress::address_type_t type) 
{
    switch (type) {
        case SocketAddress::ADDRESS_TYPE_INET:
            return AF_INET;
        case SocketAddress::ADDRESS_TYPE_INET6:
            return AF_INET6;
        case SocketAddress::ADDRESS_TYPE_UNSPEC:
            return AF_UNSPEC;

        default:
            assert(!"should not reach here");
            return AF_UNSPEC;
    }
}


SocketAddress::SocketAddress() 
{
    _platform_data = reinterpret_cast<void *>(new sockaddr_storage);
    memset(_platform_data, 0, sizeof(sockaddr_storage));
    
    reinterpret_cast<sockaddr_storage *>(_platform_data)->ss_family = AF_INET;
}

SocketAddress::SocketAddress(const SocketAddress & src)
{
    _platform_data = reinterpret_cast<void *>(new sockaddr_storage);
    memcpy(_platform_data, src._platform_data, sizeof(sockaddr_storage));
}



SocketAddress::SocketAddress(const char * addrString, int port, SocketAddress::address_type_t type)
{
    _platform_data = reinterpret_cast<void *>(new sockaddr_storage);
    memset(_platform_data, 0, sizeof(sockaddr_storage));
    
    // default to ipv4 in case the following operation fails
    reinterpret_cast<sockaddr_storage *>(_platform_data)->ss_family = AF_INET;

    setAddressFromString(addrString, type);
    setPort(port);
}

SocketAddress::SocketAddress(void * platform_data)
 : _platform_data(platform_data)
{}

/*
    https://www.cnblogs.com/zpcdbky/p/5027481.html
    @赋值运算符重载函数(operator=)
    @更“正统”的叫法是“拷贝赋值运算符”(Copy Assignment Operator)
    
    一般地，赋值运算符重载函数的参数是函数所在类的const类型的引用（如上面例1），加const是因为：
        ①我们不希望在这个函数中对用来进行赋值的“原版”做任何修改。
        ②加上const，对于const的和非const的实参，函数都能接受；如果不加，就只能接受非const的实参。
    用引用是因为：
        这样可以避免在函数调用时对实参的一次拷贝，提高了效率。
*/
SocketAddress & SocketAddress::operator = (const SocketAddress &src)
{
    memcpy(_platform_data, src._platform_data, sizeof(sockaddr_storage));
    return *this;
}


SocketAddress::~SocketAddress()
{
    delete reinterpret_cast<sockaddr_storage *>(_platform_data);
}

SocketAddress::address_type_t SocketAddress::getAddressType() const
{
    switch(reinterpret_cast<const sockaddr_storage *>(_platform_data)->ss_family) {
        case AF_INET:
            return ADDRESS_TYPE_INET;
        case AF_INET6:
            return ADDRESS_TYPE_INET6;
        default:
            assert(!"should not reach here");
            return ADDRESS_TYPE_INET;
    }
}
/*
在 C++ 中，const 修饰成员函数既可以放在函数声明的末尾（后置 const），也可以放在函数声明的开始（前置 const）。这两种位置的 const 有一些细微的区别。
1. 后置 const：
   将 const 放在函数声明的末尾，表示该成员函数是一个常量成员函数。这意味着在这个函数中，不能修改成员变量的值（除非成员变量被声明为 mutable）。后置 const 的目的是为了告诉编译器和其他开发者，这个函数不会修改对象的状态，可以在常量对象（const 对象）上调用。
   例如：
   ```
   void someFunction() const;
   ```
2. 前置 const：
   将 const 放在函数声明的开始，表示该成员函数的返回值是一个常量。这意味着函数的返回值不能被修改，可以视为一个 "const 返回类型"。前置 const 的目的是将 const 应用于函数的返回值，以防止返回值被修改。
   例如：
   ```
   const int someFunction();
   ```
在这个例子中，返回值 int 被 const 修饰，意味着返回的整数值是常量，不允许被修改。
需要注意的是，无论是后置 const 还是前置 const，它们都不影响函数的参数或局部变量的修改能力。
综上所述，const 放在函数声明的末尾表示常量成员函数，表示函数不会修改对象的成员变量；而 const 放在函数声明的开始表示函数的返回值是一个常量，表示返回值不能被修改。
*/
int SocketAddress::getPort() const
{
    switch (getAddressType()) {
        case ADDRESS_TYPE_INET:
            return (int)ntohs(reinterpret_cast<const sockaddr_in *>(_platform_data)->sin_port);
        case ADDRESS_TYPE_INET6:
            return (int)ntohs(reinterpret_cast<const sockaddr_in6 *>(_platform_data)->sin6_port);
        default:
            return 0;
    }
}

u_result SocketAddress::setPort(int port)
{
    switch (getAddressType()) {
        case ADDRESS_TYPE_INET:
            reinterpret_cast<sockaddr_in *>(_platform_data)->sin_port = htons((short)port);
            break;
        case ADDRESS_TYPE_INET6:
            reinterpret_cast<sockaddr_in6 *>(_platform_data)->sin6_port = htons((short)port);
            break;
        default:
            return RESULT_OPERATION_FAIL;
    }
    return RESULT_OK;
}

u_result SocketAddress::setAddressFromString(const char * address_string,  SocketAddress::address_type_t type)
{
    int ans = 0;
    int prevPort = getPort();
    switch (type) {
        case ADDRESS_TYPE_INET:
            reinterpret_cast<sockaddr_storage *>(_platform_data)->ss_family = AF_INET;
            ans = inet_pton(AF_INET, 
                            address_string, 
                            &reinterpret_cast<sockaddr_in *>(_platform_data)->sin_addr);
        break;


        case ADDRESS_TYPE_INET6:
            
            reinterpret_cast<sockaddr_storage *>(_platform_data)->ss_family = AF_INET6;
            ans = inet_pton(AF_INET6, 
                            address_string, 
                            &reinterpret_cast<sockaddr_in6  *>(_platform_data)->sin6_addr);
        break;

        default:
            return RESULT_INVALID_DATA;

    }
    setPort(prevPort);

    return ans<=0?RESULT_INVALID_DATA:RESULT_OK;
}


u_result SocketAddress::getAddressAsString(char * buffer, size_t buffersize) const
{
    int net_family = reinterpret_cast<const sockaddr_storage *>(_platform_data)->ss_family;
    const char *ans = NULL;
    switch (net_family) {
        case AF_INET:
            ans = inet_ntop(net_family, &reinterpret_cast<const sockaddr_in *>(_platform_data)->sin_addr,
                            buffer, buffersize);
        break;

        case AF_INET6:
            ans = inet_ntop(net_family, &reinterpret_cast<const sockaddr_in6 *>(_platform_data)->sin6_addr,
                            buffer, buffersize);

        break;
    }
    return ans==NULL?RESULT_OPERATION_FAIL:RESULT_OK;
}



size_t SocketAddress::LoopUpHostName(const char * hostname, const char * sevicename, std::vector<SocketAddress> &addresspool , bool performDNS, SocketAddress::address_type_t type)
{
    struct addrinfo hints;
    struct addrinfo *result;
    int ans;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = _halAddrTypeToOSType(type);
    hints.ai_flags = AI_PASSIVE;

    if (!performDNS) {
        hints.ai_family |= AI_NUMERICSERV | AI_NUMERICHOST;
    
    }

    ans = getaddrinfo(hostname, sevicename, &hints, &result);

    addresspool.clear();

    if (ans != 0) {
        // hostname loopup failed
        return 0;
    }

    
    for (struct addrinfo * cursor = result; cursor != NULL; cursor = cursor->ai_next) {
        if (cursor->ai_family == ADDRESS_TYPE_INET || cursor->ai_family == ADDRESS_TYPE_INET6) {
            sockaddr_storage * storagebuffer = new sockaddr_storage;
            assert(sizeof(sockaddr_storage) >= cursor->ai_addrlen);
            memcpy(storagebuffer, cursor->ai_addr, cursor->ai_addrlen);
            addresspool.push_back(SocketAddress(storagebuffer));
        }
    }

    
    freeaddrinfo(result);

    return addresspool.size();
}


u_result SocketAddress::getRawAddress(_u8 * buffer, size_t bufferSize) const
{
     switch (getAddressType()) {
        case ADDRESS_TYPE_INET:
            if (bufferSize < sizeof(in_addr::s_addr)) return RESULT_INSUFFICIENT_MEMORY;

            memcpy(buffer, &reinterpret_cast<const sockaddr_in *>(_platform_data)->sin_addr.s_addr, sizeof(reinterpret_cast<const sockaddr_in *>(_platform_data)->sin_addr.s_addr));

            
            break;
        case ADDRESS_TYPE_INET6:
            if (bufferSize < sizeof(in6_addr::s6_addr)) return RESULT_INSUFFICIENT_MEMORY;
            memcpy(buffer, reinterpret_cast<const sockaddr_in6 *>(_platform_data)->sin6_addr.s6_addr, sizeof(reinterpret_cast<const sockaddr_in6 *>(_platform_data)->sin6_addr.s6_addr));

            break;
        default:
            return RESULT_OPERATION_FAIL;
    }
    return RESULT_OK;
}


void SocketAddress::setLoopbackAddress(SocketAddress::address_type_t type)
{

    int prevPort = getPort();
    switch (type) {
        case ADDRESS_TYPE_INET:
            {
                sockaddr_in * addrv4 = reinterpret_cast<sockaddr_in *>(_platform_data);
                addrv4->sin_family = AF_INET;
                addrv4->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            }
            break;
        case ADDRESS_TYPE_INET6:
            {
                sockaddr_in6  * addrv6 = reinterpret_cast<sockaddr_in6  *>(_platform_data);
                addrv6->sin6_family = AF_INET6;
                addrv6->sin6_addr = in6addr_loopback;

            }
            break;
        default:
            return;
    }

    setPort(prevPort);
}

void SocketAddress::setBroadcastAddressIPv4()
{

    int prevPort = getPort();
    sockaddr_in * addrv4 = reinterpret_cast<sockaddr_in *>(_platform_data);
    addrv4->sin_family = AF_INET;
    addrv4->sin_addr.s_addr = htonl(INADDR_BROADCAST);
    setPort(prevPort);

}

void SocketAddress::setAnyAddress(SocketAddress::address_type_t type)
{
    int prevPort = getPort();
    switch (type) {
        case ADDRESS_TYPE_INET:
            {
                sockaddr_in * addrv4 = reinterpret_cast<sockaddr_in *>(_platform_data);
                addrv4->sin_family = AF_INET;
                addrv4->sin_addr.s_addr = htonl(INADDR_ANY);
            }
            break;
        case ADDRESS_TYPE_INET6:
            {
                sockaddr_in6  * addrv6 = reinterpret_cast<sockaddr_in6  *>(_platform_data);
                addrv6->sin6_family = AF_INET6;
                addrv6->sin6_addr = in6addr_any;

            }
            break;
        default:
            return;
    }

    setPort(prevPort);


}


}}



///--------------------------------


namespace rp { namespace arch { namespace net{ 

using namespace rp::net;

class _single_thread StreamSocketImpl : public StreamSocket
{
public:

    StreamSocketImpl(int fd)
        : _socket_fd(fd)
    {
        assert(fd>=0);
        int bool_true = 1;
        ::setsockopt( _socket_fd, SOL_SOCKET, SO_REUSEADDR , (char *)&bool_true, sizeof(bool_true) );

        enableNoDelay(true);
        this->setTimeout(DEFAULT_SOCKET_TIMEOUT, SOCKET_DIR_BOTH);
    }

    virtual ~StreamSocketImpl() 
    {
        close(_socket_fd);
    }

    virtual void dispose()
    {
        delete this;
    }


    virtual u_result bind(const SocketAddress & localaddr)
    {
        const struct sockaddr * addr = reinterpret_cast<const struct sockaddr *>(localaddr.getPlatformData());
        assert(addr);
        int ans = ::bind(_socket_fd, addr, sizeof(sockaddr_storage));
        if (ans) {
            return RESULT_OPERATION_FAIL;
        } else {
            return RESULT_OK;
        }
    }

    virtual u_result getLocalAddress(SocketAddress & localaddr)
    {
        struct sockaddr * addr = reinterpret_cast<struct sockaddr *>( const_cast<void *>(localaddr.getPlatformData())); //donnot do this at home...
        assert(addr);

        size_t actualsize =  sizeof(sockaddr_storage);
        int ans = ::getsockname(_socket_fd, addr, (socklen_t*)&actualsize);

        assert(actualsize <= sizeof(sockaddr_storage));
        assert(addr->sa_family == AF_INET || addr->sa_family == AF_INET6);

        return ans?RESULT_OPERATION_FAIL:RESULT_OK;
    }

    virtual u_result setTimeout(_u32 timeout, socket_direction_mask msk)
    {
        int ans;
        timeval tv;
        tv.tv_sec = timeout / 1000; 
        tv.tv_usec = (timeout % 1000) * 1000; 

        if (msk & SOCKET_DIR_RD) {
             ans = ::setsockopt( _socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv) );
             if (ans) return RESULT_OPERATION_FAIL;
        }

        if (msk & SOCKET_DIR_WR) {
             ans = ::setsockopt( _socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv) );
             if (ans) return RESULT_OPERATION_FAIL;
        }

        return RESULT_OK;
    }
  
    virtual u_result connect(const SocketAddress & pairAddress)
    {
        const struct sockaddr * addr = reinterpret_cast<const struct sockaddr *>(pairAddress.getPlatformData());
        int ans = ::connect(_socket_fd, addr, sizeof(sockaddr_storage));
        if (!ans) return RESULT_OK;


        switch (errno) {
            case EAFNOSUPPORT:
                return RESULT_OPERATION_NOT_SUPPORT;
#if 0
            case EINPROGRESS:
                return RESULT_OK; //treat async connection as good status
#endif
            case ETIMEDOUT:
                return RESULT_OPERATION_TIMEOUT;
            default:
                return RESULT_OPERATION_FAIL;
        }
    }
      
    virtual u_result listen(int backlog)
    {
        int ans = ::listen( _socket_fd,   backlog);

        return ans?RESULT_OPERATION_FAIL:RESULT_OK;
    }

    virtual StreamSocket * accept(SocketAddress * pairAddress) 
    {
        size_t addrsize;
        addrsize = sizeof(sockaddr_storage);
        int pair_socket = ::accept( _socket_fd, pairAddress?reinterpret_cast<struct sockaddr *>(const_cast<void *>(pairAddress->getPlatformData())):NULL
            , (socklen_t*)&addrsize);

        if (pair_socket>=0) {
            return new StreamSocketImpl(pair_socket);
        } else {
            return NULL;
        }
    }

    virtual u_result waitforIncomingConnection(_u32 timeout)
    {
        return waitforData(timeout);
    }

    virtual u_result send(const void * buffer, size_t len) 
    {
        size_t ans = ::send( _socket_fd, buffer, len, MSG_NOSIGNAL);
        if (ans == len) {
            return RESULT_OK;
        } else {
            switch (errno) {
                case EAGAIN:
#if EWOULDBLOCK!=EAGAIN
                case EWOULDBLOCK:
#endif
                    return RESULT_OPERATION_TIMEOUT;
                default:
                    return RESULT_OPERATION_FAIL;
            }
        }
        
    }


    virtual u_result recv(void *buf, size_t len, size_t & recv_len)
    {
        size_t ans = ::recv( _socket_fd, buf, len, 0);
        if (ans == (size_t)-1) {
            recv_len = 0;  

            switch (errno) {
                case EAGAIN:
#if EWOULDBLOCK!=EAGAIN
                case EWOULDBLOCK:
#endif
                    return RESULT_OPERATION_TIMEOUT;
                default:
                    return RESULT_OPERATION_FAIL;
            }

            

        } else {
            recv_len = ans;
            return RESULT_OK;
        }
    }

#if 0
    virtual u_result recvNoWait(void *buf, size_t len, size_t & recv_len)
    {
        size_t ans = ::recv( _socket_fd, buf, len, MSG_DONTWAIT);
        if (ans == (size_t)-1) {
            recv_len = 0;  
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return RESULT_OK; 
            } else {
                return RESULT_OPERATION_FAIL;
            }


        } else {
            recv_len = ans;
            return RESULT_OK;
        }

    }
#endif

    virtual u_result getPeerAddress(SocketAddress & peerAddr)
    {
        struct sockaddr * addr = reinterpret_cast<struct sockaddr *>(const_cast<void *>(peerAddr.getPlatformData())); //donnot do this at home...
        assert(addr);
        size_t actualsize =  sizeof(sockaddr_storage);
        int ans = ::getpeername(_socket_fd, addr, (socklen_t*)&actualsize);

        assert(actualsize <=  sizeof(sockaddr_storage));
        assert(addr->sa_family == AF_INET || addr->sa_family == AF_INET6);

        return ans?RESULT_OPERATION_FAIL:RESULT_OK;

    }

    virtual u_result shutdown(socket_direction_mask mask)
    {
        int shutdw_opt ;

        switch (mask) {
            case SOCKET_DIR_RD:
                shutdw_opt = SHUT_RD;
                break;
            case SOCKET_DIR_WR:
                shutdw_opt = SHUT_WR;
                break;
            case SOCKET_DIR_BOTH:
            default:
                shutdw_opt = SHUT_RDWR;
        }

        int ans = ::shutdown(_socket_fd, shutdw_opt);
        return ans?RESULT_OPERATION_FAIL:RESULT_OK;
    }

    virtual u_result enableKeepAlive(bool enable)
    {
        int bool_true = enable?1:0;
        return ::setsockopt( _socket_fd, SOL_SOCKET, SO_KEEPALIVE , &bool_true, sizeof(bool_true) )?RESULT_OPERATION_FAIL:RESULT_OK;
    }

    virtual u_result enableNoDelay(bool enable ) 
    {
        int bool_true = enable?1:0;
        return ::setsockopt( _socket_fd, IPPROTO_TCP, TCP_NODELAY,&bool_true, sizeof(bool_true) )?RESULT_OPERATION_FAIL:RESULT_OK;
    }

    virtual u_result waitforSent(_u32 timeout ) 
    {
        fd_set wrset;
        FD_ZERO(&wrset);
        FD_SET(_socket_fd, &wrset);

        timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        int ans = ::select(_socket_fd+1, NULL, &wrset, NULL, &tv);

        switch (ans) {
            case 1:
                // fired
                return RESULT_OK;
            case 0:
                // timeout
                return RESULT_OPERATION_TIMEOUT;
            default:
                delay(0); //relax cpu
                return RESULT_OPERATION_FAIL;
        }
    }

    virtual u_result waitforData(_u32 timeout )
    {
        fd_set rdset;
        FD_ZERO(&rdset);
        FD_SET(_socket_fd, &rdset);

        timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        int ans = ::select(_socket_fd+1, &rdset, NULL, NULL, &tv);

        switch (ans) {
            case 1:
                // fired
                return RESULT_OK;
            case 0:
                // timeout
                return RESULT_OPERATION_TIMEOUT;
            default:
                delay(0); //relax cpu
                return RESULT_OPERATION_FAIL;
        }
    }

protected:
    int  _socket_fd;


};


class _single_thread DGramSocketImpl : public DGramSocket
{
public:

    DGramSocketImpl(int fd)
        : _socket_fd(fd)
    {
        assert(fd>=0);
        int bool_true = 1;
        ::setsockopt( _socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST , (char *)&bool_true, sizeof(bool_true) );
        setTimeout(DEFAULT_SOCKET_TIMEOUT, SOCKET_DIR_BOTH);
    }

    virtual ~DGramSocketImpl() 
    {
        close(_socket_fd);
    }

    virtual void dispose()
    {
        delete this;
    }


    virtual u_result bind(const SocketAddress & localaddr)
    {
        const struct sockaddr * addr = reinterpret_cast<const struct sockaddr *>(localaddr.getPlatformData());
        assert(addr);
        int ans = ::bind(_socket_fd, addr, sizeof(sockaddr_storage));
        if (ans) {
            return RESULT_OPERATION_FAIL;
        } else {
            return RESULT_OK;
        }
    }

    virtual u_result getLocalAddress(SocketAddress & localaddr)
    {
        struct sockaddr * addr = reinterpret_cast<struct sockaddr *>(const_cast<void *>((localaddr.getPlatformData()))); //donnot do this at home...
        assert(addr);

        size_t actualsize =  sizeof(sockaddr_storage);
        int ans = ::getsockname(_socket_fd, addr, (socklen_t*)&actualsize);

        assert(actualsize <= sizeof(sockaddr_storage));
        assert(addr->sa_family == AF_INET || addr->sa_family == AF_INET6);

        return ans?RESULT_OPERATION_FAIL:RESULT_OK;
    }

    virtual u_result setTimeout(_u32 timeout, socket_direction_mask msk)
    {
        int ans;
        timeval tv;
        tv.tv_sec = timeout / 1000; 
        tv.tv_usec = (timeout % 1000) * 1000; 

        if (msk & SOCKET_DIR_RD) {
             ans = ::setsockopt( _socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv) );
             if (ans) return RESULT_OPERATION_FAIL;
        }

        if (msk & SOCKET_DIR_WR) {
             ans = ::setsockopt( _socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv) );
             if (ans) return RESULT_OPERATION_FAIL;
        }

        return RESULT_OK;
    }
  

    virtual u_result waitforSent(_u32 timeout ) 
    {
        fd_set wrset;
        FD_ZERO(&wrset);
        FD_SET(_socket_fd, &wrset);

        timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        int ans = ::select(_socket_fd+1, NULL, &wrset, NULL, &tv);

        switch (ans) {
            case 1:
                // fired
                return RESULT_OK;
            case 0:
                // timeout
                return RESULT_OPERATION_TIMEOUT;
            default:
                delay(0); //relax cpu
                return RESULT_OPERATION_FAIL;
        }
    }

    virtual u_result waitforData(_u32 timeout )
    {
        fd_set rdset;
        FD_ZERO(&rdset);
        FD_SET(_socket_fd, &rdset);

        timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        int ans = ::select(_socket_fd+1, &rdset, NULL, NULL, &tv);

        switch (ans) {
            case 1:
                // fired
                return RESULT_OK;
            case 0:
                // timeout
                return RESULT_OPERATION_TIMEOUT;
            default:
                delay(0); //relax cpu
                return RESULT_OPERATION_FAIL;
        }
    }

    virtual u_result sendTo(const SocketAddress & target, const void * buffer, size_t len)
    {
        const struct sockaddr* addr = reinterpret_cast<const struct sockaddr*>(target.getPlatformData());
        assert(addr);
        size_t ans = ::sendto( _socket_fd, buffer, len, 0, addr, sizeof(sockaddr_storage));
        if (ans != (size_t)-1) {
            assert(ans == len);
            return RESULT_OK;
        } else {
            switch (errno) {
                case EAGAIN:
#if EWOULDBLOCK!=EAGAIN
                case EWOULDBLOCK:
#endif
                    return RESULT_OPERATION_TIMEOUT;

                case EMSGSIZE:
                    return RESULT_INVALID_DATA;
                default:
                    return RESULT_OPERATION_FAIL;
            }
        
        }

    }

    virtual u_result setPairAddress(const SocketAddress* pairAddress)
    {
        sockaddr_storage unspecAddr;
        unspecAddr.ss_family = AF_UNSPEC;

        const struct sockaddr* addr = pairAddress ? reinterpret_cast<const struct sockaddr*>(pairAddress->getPlatformData()) : reinterpret_cast<const struct sockaddr*>(&unspecAddr);
        int ans = ::connect(_socket_fd, addr, (int)sizeof(sockaddr_storage));
        return ans ? RESULT_OPERATION_FAIL : RESULT_OK;

    }
    
    virtual u_result clearRxCache()
    {
        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        fd_set rdset;
        FD_ZERO(&rdset);
        FD_SET(_socket_fd, &rdset);

        int res = -1;
        char recv_data[2];
        memset(recv_data, 0, sizeof(recv_data));
        while (true) {
            res = select(FD_SETSIZE, &rdset, nullptr, nullptr, &tv);
            if (res == 0) break;
            recv(_socket_fd, recv_data, 1, 0);
        }
        return RESULT_OK;
    }

    virtual u_result recvFrom(void *buf, size_t len, size_t & recv_len, SocketAddress * sourceAddr)
    {
        struct sockaddr * addr = (sourceAddr?reinterpret_cast<struct sockaddr *>(const_cast<void *>(sourceAddr->getPlatformData())):NULL);
        size_t source_addr_size = (sourceAddr?sizeof(sockaddr_storage):0);

        size_t ans = ::recvfrom( _socket_fd, buf, len, 0, addr, (socklen_t*)&source_addr_size);
        if (ans == (size_t)-1) {
            recv_len = 0;  
            switch (errno) {
                case EAGAIN:
#if EWOULDBLOCK!=EAGAIN
                case EWOULDBLOCK:
#endif
                    return RESULT_OPERATION_TIMEOUT;
                default:
                    return RESULT_OPERATION_FAIL;
            }

        } else {
            recv_len = ans;
            return RESULT_OK;
        }

    }

#if 0
    virtual u_result recvFromNoWait(void *buf, size_t len, size_t & recv_len, SocketAddress * sourceAddr)
    {
        struct sockaddr * addr = (sourceAddr?reinterpret_cast<struct sockaddr *>(const_cast<void *>(sourceAddr->getPlatformData())):NULL);
        size_t source_addr_size = (sourceAddr?sizeof(sockaddr_storage):0);


        size_t ans = ::recvfrom( _socket_fd, buf, len, MSG_DONTWAIT, addr, &source_addr_size);

        if (ans == (size_t)-1) {
            recv_len = 0;  
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return RESULT_OK; 
            } else {
                return RESULT_OPERATION_FAIL;
            }


        } else {
            recv_len = ans;
            return RESULT_OK;
        }

    }
#endif
    
protected:
    int  _socket_fd;

};


}}}


namespace rp { namespace net{ 


static inline int _socketHalFamilyToOSFamily(SocketBase::socket_family_t family)
{
    switch (family) {
        case SocketBase::SOCKET_FAMILY_INET:
            return AF_INET;
        case SocketBase::SOCKET_FAMILY_INET6:
            return AF_INET6;
        case SocketBase::SOCKET_FAMILY_RAW:
            return AF_PACKET;
        default:
            assert(!"should not reach here");
            return AF_INET; // force treating as IPv4 in release mode
    }

}

StreamSocket * StreamSocket::CreateSocket(SocketBase::socket_family_t family)
{
    if (family == SOCKET_FAMILY_RAW) return NULL;


    int socket_family = _socketHalFamilyToOSFamily(family);
    int socket_fd = ::socket(socket_family, SOCK_STREAM, 0);
    if (socket_fd == -1) return NULL;

    StreamSocket * newborn = static_cast<StreamSocket *>(new rp::arch::net::StreamSocketImpl(socket_fd));
    return newborn;

}


DGramSocket * DGramSocket::CreateSocket(SocketBase::socket_family_t family)
{
    int socket_family = _socketHalFamilyToOSFamily(family);


    int socket_fd = ::socket(socket_family, (family==SOCKET_FAMILY_RAW)?SOCK_RAW:SOCK_DGRAM, 0);
    if (socket_fd == -1) return NULL;

    DGramSocket * newborn = static_cast<DGramSocket *>(new rp::arch::net::DGramSocketImpl(socket_fd));
    return newborn;

}


}}

