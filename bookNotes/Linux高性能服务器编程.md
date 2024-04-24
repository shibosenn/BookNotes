# TCP/IP协议族详解

## TCP/IP协议族

<img src="https://cdn.jsdelivr.net/gh/luogou/cloudimg/data/20211214194212.png" alt="image-20211214194212219" style="zoom:67%" />

## TCP协议为应用层提供可靠的、面向连接的和基于流的服务

- 可靠

  使用超时重传(发送端在发送报文后启动 一个定时器，若定时器在规定时间内没有收到应答就会重发该报文)，发送应答机制(发送端发送的每个TCP报文都必须得到接收方的应答)等方式来确保数据包被正确的发送到目的地。最后还会对收到的TCP报文进行重排、整理然后再交付给应用层。

- 面向连接

  使用TCP链接的双方必须要建立连接，并在内核中为该链接维持一些必要的数据结构，比如度写缓冲区，定时器等

- 基于流

  基于流的数据没有边界长度限制，源源不断的从通信的一端流入另一端。发送端可以逐个字节的向数据流中写入数据，接收端也可以逐字节的将他们读出。

### TCP报文封装的过程

<img src="https://cdn.jsdelivr.net/gh/luogou/cloudimg/data/20211214192824.png" alt="image-20211214192824056" style="zoom:67%"/>

上图以发送为例来介绍这个过程：

1. 应用程序通过调用send/write向TCP写入数据
2. 从用户空间进入到内核空间
3. 将应用缓冲区的数据复制到内核TCP发送缓冲区，同时在TCP协议中还要加上TCP头，还有各种定时器函数
4. 将TCP报文段即TCP头部信息和TCP内核缓冲区数据发送到IP协议模块
5. IP模块进行封装，加上自己的IP头

### TCP状态转移

<img src="https://cdn.jsdelivr.net/gh/luogou/cloudimg/data/20211219171021.png" alt="image-20211219165514215" style="width:600px;height:360px;" />

#### 服务器状态转移

1. 服务器通过listen系统调用进入LISTEN（监听）状态，被动的等待客户端的连接

2. 服务器监听到某个请求(收到同步报文段)，进入到SYN_RCVD状态，就会将该连接放入内核等待队列中，称为半连接队列。同时向客户端发送带SYN标志、ACK标志的确认报文段。

3. 如果服务器成功的接收到客户端发送过来的确认报文段，则该连接进入到ESTABLISHED状态，双方可以进行双向数据传输。

4. 当客户端主动关闭连接的时候，服务端收到FIN关闭报文，进入到CLOSE_WAIT状态的同时发送确认报文。这个状态就是为了等待服务器应用程序关闭连接，此时客户端到服务器的已经关了，但是服务器到客户端的还没有关掉。

   当服务器检测到客户端关闭连接后，就会再发送一个ack报文段给客户端，这个时候就进入了LAST_ACK状态，等待客户端对结束报文的最后一次确认，一旦确认完成连接就彻底关闭。

#### 客户端的状态转移

1. 客户端通过connect系统调用主动与服务器建立连接、首先给服务器发送一个同步报文，使得连接进入SYN_SENT状态。此后返回失败有两种可能

   - 目标端口不存在，或者该端口仍处于TIME_WAIT状态的连接所占用，则服务器给客户端发送一个复位报文段，表示连接失败
   - 目标端口存在，但是connect在超时时间内未收到来自服务器的确认报文

   调用失败使得进入CLOSED状态

   如果成功则进入ESTABLISHED状态

2. 客户端主动执行关闭，发送结束报文段给服务器端，同时进入FIN_WAIT_1状态。

3. 客户端收到了来自服务器的确认报文段，则进入FIN_WAIT_2状态。当客户端处于FIN_WAIT_2状态的时候服务器处于CLOSE_WAIT状态，可能会发生半关闭状态。

4. 随后服务器又发送ack报文段，则客户端进入TIME_WAIT状态，

> 注意，处于FIN_WAIT_2状态的客户端需要等待服务器发送带有FIN、ACK的报文段后才能转移到TIME_WAIT状态，否则就会一直停留在FIN_WAIT_2状态。如果不是为了在半关闭状态下接收数据，长时间停留在这个状态没有一处。
>
> 连接停留在FIN_WAIT_2状态可能发生在客户端执行半关闭后未等服务器关闭连接就强行退出了。此时客户端由内核接管，成为孤儿连接。内核中有指定孤儿连接的数目和孤儿连接的生存时间。

### TCP协议的半关闭状态

TCP 半关闭状态是指在一个 TCP 连接中，其中一端（通常为客户端）关闭了其发送数据的能力，但仍能够接收数据，而另一端仍然可以发送数据，直到双方都关闭连接。半关闭状态允许数据的单向传输，可以用于实现单向通信或客户端发送完数据后等待服务器的响应。

### TIME_WAIT状态

客户端收到了来自服务器的带有FIN、ACK的结束报文段后并没有直接关闭，而是进入了TIME_WAIT状态。

在这个状态中客户端只有等待2MSL才能完全关闭。

TIEM_WAIT存在的原因有以下两点：

- 可靠的终止TCP连接

  当客户端向服务器发送的最后一个确认报文段丢失的时候，也就是说服务器没有收到客户端的确认报文段（TCP状态转移中的图），那么服务器将重新发送带有FIN、ACK的结束报文段。这样客户端必须停留在某个状态来接收服务器发送过来的结束报文段才行，所以要有TIME_WAIT状态。

- 保证让迟来的TCP报文段有足够的时间被识别并丢弃

  一个TCP端口不能同时被打开多次，当TCP连接的客户端处于TIME_WAIT的时候，我们没有办法在被占的端口建立新连接。如果不存在TIME_WAIT状态，则应用程序就可以建立一个和刚关闭的连接相似的连接（相同的IP、端口号），那么这个新连接就会收到可能属于原来连接的数据，这显然是不应该发生的。

  TCP报文最大生存时间是MSL，所以坚持2MSL时间的TIME_WAIT可以保证链路上未被收到的，延迟的TCP报文段都消失，这样当相似新连接建立的时候就不会受到原来属于就连接的数据报文

  不过，由于客户端使用系统自动分配的临时端口号来建立连接，所以一般不用考虑这个问题。

### TCP复位报文段

在某些情况下，TCP连接的一端会向另一端发送携带RST标志的报文段，即复位报文段，以通知对方关闭连接或重新建立连接。主要有以下三种情况：

- 1、访问不存在端口

当访问不存在的端口时候，服务器将发送一个复位报文段。因为复位报文段接收通告窗口大小为0，所以受到复位报文段的一端应该关闭连接或者重新连接，而不能回应这个复位报文段。

> 当客户端向服务器某个端口发起连接的时候，若端口处于TIME_WAIT状态，客户端程序也会收到复位报文段。

- 2、异常终止连接

TCP提供了一个异常终止连接的方法：给对方发送一个复位报文段，一旦发送了复位报文段，发送端所有排队等待发送的数据都将被丢弃。

- 3、处理半打开连接

情况出现原因：客户端突然关闭了连接，但是服务器没有接收到结束报文段，此时服务器还维持着原来的连接，而客户端即使重启也没有原来的连接。这种状		态称为半打开状态，处于这种状态的连接称为半打开连接。如果服务器往半打开连接中写入数据，则发送端会回应一个复位报文段（客户端被内核接管）。

### TCP的数据流分类

TCP所携带的应用程序数据按照长度可以分为两种：交互数据和成块数据。

- 交互数据仅包含很少的字节，交互数据对实时性要求很高，例如ssh等

  交互数据相关的有延迟确认、nagle算法

- 成块数据则通常为TCP报文段允许的最大长度数据，对传输效率要求高，比如FTP

#### TCP延迟确认

从服务器视角来看，若TCP是交互数据块的话，证明数据量很少，这种情况下服务器没有必要收到一个数据报就确认一个。而是在一段时间延迟后查看服务器端是否有数据需要发送，有的话确认消息就和数据一起发出。**由于服务器对客户请求处理的很快，因此当发送确认报文段的时候总会有数据和这个确认信息一起发出。**

延迟确认可以减少TCP报文的发送数量，**对于客户端来说，由于用户输入数据的速度明显慢于客户端处理数据的速度，因此客户端的确认报文段总是不携带任何应用数据**。因此本质上还是想减少网络中传输的TCP报文段数量，防止拥塞。有一个算法也是为了防止拥塞。

#### nagle算法

在TCP协议中，当数据发送端向接收端发送数据时，数据会被分成一系列TCP包进行传输。Nagle算法的作用就是将小的数据包合并成一个大的，从而减少网络流量和提高吞吐量。具体来说，当一个TCP包发送出去后，如果发送端还有需要发送的数据，则不立即发送该数据，而是将其缓存起来。直到发现以下条件之一才会发送数据：

1.已经缓存的数据量超过了一个指定大小。

2.距离上次发送数据已经过了一定的时间（称为“延迟确认时间”）。

3.收到了之前发送的数据的确认信号。

## UDP协议提供不可靠的、无连接的和基于数据报的服务

- 不可靠

  UDP协议无法保证数据能够从发送端正确的传送到目的端口。如果数据在中途丢失或目的端设备会通过数据校验发现数据错误而将其丢失，则UDP协议只是简单地通知应用程序发送失败

- 无连接

   通信双方不能保持一个长久的链接，因此应用程序每次发送数据都要明确指明接收端的地址

- 基于数据报

  每个UDP数据报都有长度，接收端必须以该长度为最小单位将内容从缓存中读出，否则数据会被截断。

### UDP报文封装的注意事项

UDP封装与TCP类似，但是UDP无需为应用层数据保存一个副本，因为提供的是不可靠的服务。当一个UDP数据报被成功发送之后，UDP内核缓冲区就会把该数据丢弃。如果发送端应用程序检测到该UDP报文没有被正确接收并打算重发该数据的时候，由于没有缓存，应用程序需要重新将发送数据从用户空间拷贝到内核发送缓冲区。

## IP协议的特点

IP协议是TCP/IP协议族的动力，为上层提供了无状态、无连接、不可靠的服务

- 无状态

  指进行IP通信的双方不同步传输数据的状态信息。所以所有IP数据报的发送、传输和接受都是独立的，没有上下文关系。

  最大的缺点是无法处理乱序和重复的IP数据报

  同时无状态服务也有优点：简单和高效。无需为保持通信状态而分配内核资源和算力，也无需每次传递数据都携带状态信息

- 无连接

  指通信双方都不长久的维持对方的信息，因此当上层协议每次发送数据的时候都要指明对方的IP地址。

- 不可靠

  指IP协议不能够保证IP数据报准确地到达接收端，他只是承诺尽最大努力交付。因此很多情况都可能导致IP数据报发送失败。因此可靠传输都是需要上层传输层协议去考虑的问题。

# Linux网络编程基础

## 网络编程API

### 主机字节序和网络字节序

```c
#include <arpa/inet.h>

uint32_t htonl(uint32_t hostlong);		//把uint32_t类型从主机序转换到网络序
uint16_t htons(uint16_t hostshort);		//把uint16_t类型从主机序转换到网络序
uint32_t ntohl(uint32_t netlong);		//把uint32_t类型从网络序转换到主机序
uint16_t ntohs(uint16_t netshort);		//把uint16_t类型从网络序转换到主机序
```

### socket编程整体结构

```text
		被动socket(server)              主动socket(client)
---------------------------------------------------------------------------
            socket()
               |
            bind()
               |
            listen()                        socket()
               |                              |
            accept()                          |
               |                              |
               | <------------------------- connect()
               |                              |
            read() <------------------------ write()
               |                              |
            write() -----------------------> read()
               |                              |
            close()                         close()
---------------------------------------------------------------------------
```

### 存储socket地址的结构体

表示socket地址的是结构体sockaddr，定义如下：

```c
struct sockaddr    
{    
    uint8_t  sin_len;    
    sa_family_t  sin_family;    
    char sa_data[14];   //14字节，包含IP和地址 　　    
}; 
```

sockaddr的缺陷是把目标地址和端口混在一起了，所以TCP/IP协议族有两个专用的socket地址结构体如下（只介绍IPv4的）：

```c
struct sockaddr_in    
{     
    sa_family_t  sin_family;    //地址族：AF_INET 
    u_int16_t   sin_port;       //端口号，网络字节序表示   
    struct in_addr  sin_addr;   //ipv4结构体
};
struct in_addr
{
    u_int32_t s_addr;           //ipv4地址，要用网络字节序表示
};
```

> 所有专用的socket地址类型的变量在实际使用时候都需要转化为通用socket地址类型`sockaddr`（强转就行），因为所有socket编程接口使用的地址参数类型都是sockaddr！！

### IP地址转换函数

```c
#include <arpa/inet.h>

in_addr_t inet_addr(const char* strptr);
int inet_aton(const char* cp, struct in_addr* inp);
char* inet_ntoa(struct in_addr in);

int inet_pton(int af, const char* src, void* dst);
const char* inet_ntop(int af, const void* src, char* dst, socklen_t cnt);
```

`inet_addr`函数将用点分十进制字符串表示的IPv4地址转化为用网络字节序整数表示的IPv4地址。

`inet_aton`函数完成和`inet_addr`同样的功能，但是将转化结果存储于参数inp指向的地址结构中。

`inet_ntoa`函数将用网络字节序整数表示的IPv4地址转化为用点分十进制字符串表示的IPv4地址。
该函数内部用一个静态变量存储转化的结果，函数返回值指向该静态内存，因此`inet_ntoa`是不可重入的。

### socket函数

```c
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
int socket(int domain, int type, int protocol);
```

|  协议簇  |  地址族  |       描述       |
| :------: | :------: | :--------------: |
| PF_UNIX  | AF_UNIX  | UNIX本地域协议族 |
| PF_INET  | AF_INET  |  TCP/IPv4协议族  |
| PF_INET6 | AF_INET6 |  TCP/IPv6协议族  |

由于宏`AF_*`和`PF_*`定义的是完全相同的值，所以二者经常混用

|      名称      |                             含义                             |
| :------------: | :----------------------------------------------------------: |
|  SOCK_STREAM   | Tcp连接，提供序列化的、可靠的、双向连接的字节流。支持带外数据传输 |
|   SOCK_DGRAM   |               支持UDP连接（无连接状态的消息）                |
| SOCK_SEQPACKET | 序列化包，提供一个序列化的、可靠的、双向的基本连接的数据传输通道，数据长度定常。每次调用读系统调用时数据需要将全部数据读出 |
|    SOCK_RAW    |                RAW类型，提供原始网络协议访问                 |
|    SOCK_RDM    |           提供可靠的数据报文，不过可能数据会有乱序           |

### bind函数

```c
int bind( int sockfd, struct sockaddr* addr, socklen_t addrlen)
```

在创建一个socket的时候我们给socket指定了地址族，但是重点是没有给这个socket指定一个具体的地址，因此bind就是将创建的socket文件描述符和一个socket地址绑定。

对于服务器这边的代码来说，要绑定一个众所周知的IP地址和端口以等待客户连接

### listen函数

```c
int listen(int sockfd, int backlog);
```

当bind完成后并不能马上就能接受客户端连接，需要使用listen系统调用来创建一个监听队列以存放待处理的客户端连接。

`backlog`参数表示内核监听队列的最大长度，如果超过这个最大长度则服务器将不受理新的客户连接，客户端也将收到错误返回，典型值是5。如果底层协议支持重传(比如tcp协议),本次请求会被丢弃不作处理，在下次重试时期望能连接成功(下次重传的时候队列可能已经腾出空间)。

### accept函数

```c
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

sockfd参数是指执行过listen系统调用的监听socket，addr是来获取接受连接的客户端的地址信息。

accept成功的时候会返回一个新的socket，这个新socket唯一的标识了这个被接受的连接，服务端和客户端就是通过这个socket来建立连接和通信的。

> accept是一个阻塞式的函数，对于一个阻塞的套接字，要么一直阻塞直到等到想要的值。对于非阻塞套接字会调用完直接返回一个值。accept有可能返回-1，但是如果errno的值为，EAGAIN或者EWOULDBLOCK，此时需要重新调用一次accept函数。

### connect函数

```c
int connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);
```

这个函数是对于客户端来说的。`sockfd`是系统调用` socket() `返回的套接字文件描述符，`serv_addr`是保存着要连接对象目的地端口和IP地址的数据结构 `struct sockaddr_in`。

### close函数

```c
#include<unistd.h>
int close(int fd)
```

close系统调用并非总是立刻关闭一个连接，而是将fd的引用计数减一。只有当fd的引用计数为0的时候才真正关闭连接。

比如多进程中我们使用系统调用创建一个子进程，会将父进程中打开的socket的引用计数+1，因此必须在父进程和子进程中都对执行close函数才能关闭连接，也就是必须执行两次close将引用计数-2才行。

立即终止连接是如下函数：

```c
#include<sys/socket.h>
int shutdown(int fd, int howto)
```

![image-20211224202147836](https://cdn.jsdelivr.net/gh/luogou/cloudimg/data/image-20211224202147836.png)

### 数据接收(读写)

### TCP数据读写

```c
#include <sys/types.h>
#include <sys/socket.h>
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
```

需要注意的是，send实际是将应用程序的数据拷贝到内核的TCP模块的发送缓冲区，发往网络连接的具体实现是在系统内核。所以send返回成功的情况下，也不能证明数据就一定发送成功。

### UDP数据读写

```c
#include <sys/types.h>
#include <sys/socket.h>
ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr,socklen_t* addrlen);
ssize_t sendto(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr* dest_addr,socklen_t addrlen);
```

### 通用数据读取函数

socket编辑接口还提供了一堆通用的数据读写系统调用。他们不仅能用于TCP数据流，也可以用于UDP数据报

```c
#include <sys/socket.h>
ssize_t recvmsg(int sockfd,struct msghdr* msg, int flags);
ssize_t sendmsg(int sockfd,struct msghdr* msg, int flags);
```

sockfd参数指定被操作的目标socket。msg参数是msghdr结构体类型的指针，msghdr结构体的定义如下：

```c
struct msghdr
{
	void* msg_name;				//socket地址
	socklen_t msg_namelen;		//socket地址的长度
	struct iovec* msg_iov；		//分散的内存块
	int msg_iovlen;				//分算的内存块数量
	void* msg_control;			//指向辅助数据的起始位置
	socklen_t msg_controllen;	//辅助数据的大小
	int msg_flags;				//赋值函数中的flags参数，并在调用过程中更新
};
```

### socket选项设置

socket选项主要是由setsockopt和getsockopt函数完成的。

```c
#include <sys/types.h>          
#include <sys/socket.h>

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
```

### 地址信息函数

```c
#include <sys/socket.h>
/* 获取sockfd本端socket地址，并将其存储于address参数指定的内存中 */
int getsockname ( int sockfd, struct sockaddr* address, socklen_t* address_len );    
/* 获取sockfd对应的远端socket地址，其参数及返回值的含义与getsockname的参数及返回值相同 */
int getpeername ( int sockfd, struct sockaddr* address, socklen_t* address_len );   
```

```c
#include <netdb.h>
struct hostent* gethostbyname(const char* name);
struct hostent gethostbyaddr(const void* addr, size_t len, int type);
struct hostent{
     char* h_name; //主机名
     char** h_aliases;//主机别名列表，可能由多个
     int h_addrtype; //地址类型（地址族）
     int h_length; //地址长度
     char** h_addr_list;//按网络字节序列出的主机IP地址列表
 }
```

```c
//根据名称获取某个服务器的完整信息
#include<netdb.h>
struct servent* getservbyname (const char* name,const char* proto);
struct servent* getservbyport (int port ,const char* proto);
struct servent{
    char* s_name;//服务名称
    char** s_aliases;//服务的别名列表，可能多个
    int s_port;//端口号
    char* s_proto;//服务类型，通常是TCP或者UDP
}
```

```c
//将返回的主机名存储在hsot参数指向的缓存中，将服务名存储在serv参数指向的缓存中，hostlen和servlen参数分别指定这两块缓存的长度
#include<netdb.h>
int getnameinfo (const struct sockaddr* sockaddr,socklen_t addrlen,char* host,socklen_t hostlen,char* serv,socklen_t servlen,int flags);
```

## 高级I/O函数

### `pipe`

```c++
int pipe(int fd[2]);
```

### `dup`

```c++
int dup(int fd);
int dup2(int fd1, int fd2);
//通过dup和dup2创建的文件描述符并不继承原文件描述符的属性，比如close-on-exec和nonblock等
```

### `readv`

```c++
ssize_t readv(int fd, const struct iovec *vector, int count);
ssize_t writev(int fd, const struct iovec *vector, int count);
```

### `sendfile`

```c++
ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count);
//in_fd 必须是真实的文件
//out_fd必须是socket
//完全在内核中操作，避免了内核缓冲区和用户缓冲区之间的数据拷贝
```

### `mmap`

```c++
void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *start, size_t length);
```

### `splice`

```c++
ssize_t splice(int fd_in, loff_t *off_in, int fd_out, loff_t *off_out, size_t len, unsigned int flags);
//fd_in和fd_out必须至少有一个是管道文件描述符
```

```c++
//利用splic搭建零拷贝的回射服务器
int pipfd[2];
ret = pip(pipfd);
ret = splice(connfd, NULL, pipfd[1], NULL, 32768, SPLIC_F_MORE | SPLICE_F_MOVE);
ret = splice(pipfd[0], NULL, connfd, NULL, 32768, SPLIC_F_MORE | SPLICE_F_MOVE);
```

### `tee`

```c++
ssize_t tee(int fd_in, int fd_out, size_t len, unsigned int flags);
//在两个管道文件描述符之间复制数据，也是零拷贝操作，不消耗数据
```

# Linux服务器程序规范

## 高性能服务器程序框架

> 主要将服务器结构为三个模块：
>
> 1. I/O处理单元，主要有四种I/O模型和两种高效事件处理模式
> 2. 逻辑单元，怎样对数据进行处理，两种高效并发模式和有限状态机
> 3. 存储单元，将数据存起来

### 服务器模型概述

TCPIP协议在最开始是没有客户端和服务端的概念的，但是现实中我们的很多应用都需要服务器提供服务然后客户端去访问这些服务，数据资源被提供者所垄断。

- C/S模型

  ![image-20220103104448168](https://s2.loli.net/2022/01/03/MS5jhf9gYQlPKVd.png)

  服务器启动之后，首先创建一个或者多个监听socket，然后调用bind函数将其绑定到服务器的相关端口上，然后调用listen函数等待客户端的连接。服务器运行稳定之后，客户端就调用connect函数向服务器发起连接请求。由于客户请求的到达是随机的异步事件，因此服务器要用某种IO模型来监听这一事件。当监听到来自客户端的链接请求之后就将请求放入接收队列中，然后服务器端调用accept函数接受它，并分配一个逻辑单元为新的连接服务，连接单元可以使新创建的子进程，子线程或其他。

  优点是适合资源相对集中的场合，并且实现简单

  缺点是服务器是通信的中心，当访问量过大时，所有客户得到响应的速度会变慢。

- P2P模型

  此模型摒弃了以服务器为中心的格局，让网络中的所有主机重新回归对等地位。
  
  使得每台机器在消耗服务的同时也给别人提供服务，这样资源能够得到充分自由的共享。
  
  <img src="https://s2.loli.net/2022/01/03/EaYfiSc5PbGZn7u.png" alt="img" style="zoom:80%;" /><img src="https://img-blog.csdn.net/20170903221006945?watermark/2/text/aHR0cDovL2Jsb2cuY3Nkbi5uZXQvcXFfMzY5NTMxMzU=/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70/gravity/Center" alt="img" style="zoom:80%;" />
  
  由于主机之间很难发现对方，因此实际使用的p2p模型带有一个专门的发现服务器，每台主机既是客户端又是服务器。

### 服务器编程框架

> <img src="https://s2.loli.net/2022/01/03/5Aqfav4SlMo3Ntx.png" alt="img" style="zoom:80%;" />

- IO处理单元

  是服务器管理客户连接的模块。有以下任务要完成：等待并接受新的客户连接，接收客户数据，将服务器响应的数据返回给客户端。

  对于服务器集群来说，IO处理单元是一个专门的接入服务器，用来实现负载均衡，然后从所有逻辑服务器中选取一台负荷最小的为新接入的客户端服务

- 逻辑单元

  一个逻辑单元通常是一个进程或者线程。它分析并处理客户端数据，然后将结果发送给IO处理单元或者直接返回给客户端

  对于服务器集群来说一个逻辑单元就是一台服务器。但一般一个服务器拥有多个逻辑单元，以实现多个任务的并行处理

- 网络存储

  可以使数据库、文件、缓存，甚至是一台独立的服务器，但不是必须的，比如ssh服务

- 请求队列

  当请求连接的客户端数量多了之后或者需要处理的数据多了之后就需要请求队列保证顺序。

  IO处理单元接受到了客户请求时候，需要以某种方式通知逻辑单元请求该连接。同样，多个逻辑单元同时访问一个存储单元时也同样需要某种机制来协调竞争条件。

  请求队列通常被实现为池的一部分。

  对服务器集群来说，请求队列是各台服务器预先建立的、静态的、永久的TCP连接。

### IO处理单元

#### IO模型

从理论上来说阻塞IO，非阻塞IO复用和非阻塞信号驱动IO都是同步IO模型。**因为IO的读写操作都是IO事件发生之后由应用程序完成的。**

对异步IO来说，用户可以直接对IO执行读写操作，这些操作告诉内核用户读写缓冲区的位置，以及IO操作完成之后内核通知应用程序的方式。异步IO的读写操作总是立即返回的，不论IO操作是否阻塞，因为真正的读写操作已经由内核接管。

同步IO模型要求用户代码自行执行IO操作，即将数据从内核缓冲去中读入用户缓冲区或者将用户缓冲区的数据写入内核缓冲区。而异步IO机制由内核执行IO操作，即数据在内核和用户缓冲区之间的操作由内核自动完成。即同步IO向应用程序通知的是IO就绪事件，异步IO向应用程序通知的是IO完成事件。

<img src="https://s2.loli.net/2022/01/03/lIcNSztYJ7hWeaD.png" alt="img" style="zoom:67%;" />

### 两种高效的事件处理模式

服务器程序通常要处理三类事件：IO事件、信号及定时事件

同步IO模型通常用于实现Reactor模式，而异步IO模型通常用于实现Proactor模式

- Reactor模式

  > 定义：主线程主要用来监听文件描述符是否有事件发生，有的话就将该事件通知工作线程即逻辑处理单元，除此之外主线程不用做任何事情。读写数据，接受新的连接、处理客户端请求均在工作线程中完成。
  >
  > ![img](https://s2.loli.net/2022/01/03/7HmlbPY6n53BCMu.png)

  工作流程如下：

  1. 主线程往epoll内核时间表中注册socket上的读就绪事件
  2. 主线程调用epoll_wait等待socket上有数据可读
  3. 当有数据可读时，epoll_wait通知主线程，然后主线程将可读事件放入请求队列中
  4. 睡眠在请求队列上的某个工作线程被唤醒，从socket中读取数据并处理客户端请求
  5. 然后往epoll内核事件表中注册socket的写事件
  6. 主线程调用epoll_wait等待socket可写
  7. 当socket可写的时候epoll_wait通知主线程，主线程将socket写事件放入请求队列
  8. 睡眠在请求队列上的某个工作线程被唤醒，王socket写入服务器处理客户端请求的结果。

- Proactor模式

  > 定义：所有IO操作都交给主线程和内核来处理，工作线程仅负责业务逻辑，而Reactor模式中工作线程不仅仅负责业务逻辑。
  >
  > ![img](https://s2.loli.net/2022/01/03/CIBVN6Fvw1iPxc8.png)
  
  工作流程如下：
  
  1. 主线程调用aio_read函数向内核注册socket上的读完成事件，并告诉内核用户读缓冲区的位置，以及读操作完成时如何通知应用程序
  2. 主线程继续处理其他逻辑
  3. 当socket上的数据被读入用户缓冲区之后，内核向应用程序发送一个信号，通知应用程序数据可用了
  4. 应用程序预先定义好的一个信号处理函数选择一个工作线程来处理客户端请求。
  5. 工作线程处理完客户端请求之后，主线程调用aio_write函数向内核注册socket上的写完成事件，并告诉内核用户写缓冲区的位置，以及操作完成后如何通知用户
  6. 主线程继续处理其他逻辑
  7. 当用户缓冲区的数据被写入socket之后，内核将向应用程序发送一个信号，通知于应用程序已经发送完毕
  8. 应用程序预先定义好的信号处理函数选择一个工作线程来做善后处理，比如是否关闭socket
  
  住现成的epoll_wait只能用来监听socket上的连接请求，而不能用来检测连接socket上的读写时间。

### 逻辑单元：两种高效并发模式

> 并发模式是指IO处理单元和多个逻辑单元之间协调完成任务的方法。

#### 半同步/半异步模式

这里面的同步异步IO操作的同步异步是不一样的。

IO模型中同步异步区分的是内核向应用程序通知的是就绪事件（同步，必须等数据都到了在通知）还是完成事件（异步，数据全部到了再回来操作），以及是应用程序来完成IO读写还是内核完成IO读写。

并发模式中的同步异步指的是程序完全按照代码顺序执行（同步）还是程序的执行需要由系统事件的驱动（异步）

<img src="https://s2.loli.net/2022/01/07/5j4lRWfZxyiYOSC.png" alt="img" style="zoom:67%;" />

> 纯异步线程：执行效率高、实时性强，因此是很多嵌入式程序选择的模型。但是编写异步程序难于调试和扩展，相对复杂因此不适合于大量并发。
>
> 纯同步线程：效率低，实时性差但是逻辑简单。
>
> 因此对于服务器这种实时性要求高，同时处理多个客户请求的应用程序应该将同步编程和异步编程结合来用。

因此引入了半同步/半异步模式如下：

> ![img](https://s2.loli.net/2022/01/08/RzaYUguKBNxDSrO.png)
>
> 在上图中异步线程用来处理IO事件（IO处理单元）、同步线程用来处理客户逻辑（逻辑单元）。
>
> 异步线程监听到客户请求之后就将其封装成请求对象并插入请求队列，请求队列通知某个工作在同步模式的工作线程读取数据并做相应的处理。选取工作线程可以顺序选取也可以随机选取。

**上面是最简单的一个模型，是最基础的，如果考虑两种事件模式和几种IO模型的话就存在很多变体，我们今天主要两个，半同步/半反应堆模式和高效的半同步/异步模式**

- 半同步/半反应堆模式

  ><img src="https://s2.loli.net/2022/01/08/Lvj8Wn9xrtUyAVi.png" alt="img" style="zoom:80%;" />
  >
  >主线程作为异步线程，负责监听所有socket上的事件。如果监听到socket上有可读事件发生同时是未连接的socket，就接受新连接的socket，然后往epoll内核事件表中注册该socket上的读写事件。如果是已经连接的socket上有读写事件发生，即有新的客户请求到来或者有数据要发送给对端，主线程就将该连接的socket插入请求队列，从队列中选取一个工作线程处理任务
  >
  >上图采用的事件处理模式是Reactor模式，工作线程自己从socket上读取客户请求和往socket中写入服务器应答。
  >
  >半同步/半反应堆模式缺点：
  >
  >1. 主线程和工作线程共享请求队列。这样需要对请求队列加锁，耗费CPU时间
  >2. 每个工作线程同一时间只能处理一个请求。如果客户数多，而工作线程较少则会在队列中堆积很多对象导致客户端响应速度变慢。如果通过增加线程数来解决这一问题，则工作线程的切换也耗费大量CPU时间

- 高效的半同步/异步模式

  > <img src="https://s2.loli.net/2022/01/08/WQaIv6ACV934meL.png" alt="img" style="zoom:80%;" />
  >
  > 和半同步/半反应堆模式不同的是主线程仅仅监听socket，而连接socket由工作线程去管理。当有新连接到来时，主线程就接受该连接同时将返回的socket连接发送给某个工作线程，此后该socket上的任何IO操作都由工作线程处理，直到客户端连接关闭。主线程向工作线程派发socket的最简单的方式就是向管道里面写数据。工作线程检测到管道有数据可读时候，就分析是否是新的客户连接到来，如果是就将新的socket上的读写事件注册到自己的epoll内核事件表中。
  >
  > 可以看到上图中每个线程（主线程和工作线程）都维持自己的事件循环，他们各自独立的监听不同的事件，在上图中每个线程都工作在异步模式。

#### 领导者/追随者模式

> 多个工作线程轮流获得事件源集合，轮流监听、分发并处理事件的一种模式。在任意时间点程序都只有一个领导者线程，负责监听IO事件。而其他线程则都是追随者，她们休眠在线程池中等待成为新的领导者。当前的领导者如果检测到IO事件，首先从线程池中选出新的领导者，再去处理IO事件。然后新的领导者等待新的IO事件，原来的领导者则处理到来的IO事件，实现并发。
>
> 包含句柄集（HandleSet）、线程集（ThreadSet）、事件处理器（EventHandler）、具体事件处理器（ConcreteEventHandler）

### 提高服务器性能的其他建议

#### 池

以空间换时间，池是一组资源的集合，这组资源在服务器启动之初就被创建出来并初始化，成为静态资源分配。当服务器正式运行，从池中直接获取相关资源，无需动态分配，这样速度会很快。池相当于服务器管理系统资源的应用层设施，避免了服务器对内核的频繁访问。

内存池、进程池、线程池、连接池等

#### 数据复制

高性能服务器应该要避免不必要的数据复制，尤其当数据复制发生在用户代码和内核之间的时候。

如果内核直接处理从socket或者文件读入的数据，则应用程序就没必要将这些数据从内核缓冲区复制到应用程序缓冲区中。（直接处理指的是应用程序不关心数据内容）

此外用户代码内部的复制也应该避免，比如两个工作进程之间传递大量消息的时候，可以考虑使用共享内存的方式去实现。

#### 上下文切换和锁

并发程序必须考虑上下文切换，即进程切换或者线程切换导致的系统开销，切换会占用CPU的时间，进而服务器用于处理业务逻辑的时间就会显得不足。

第二个就是锁，由于共享资源的原因，加锁时必要的，但是加锁会导致效率低下，因为引入的代码不仅不会对业务逻辑有帮助，而且会访问内核。

## IO复用

### select调用（跨平台）

`select`函数的优点是跨平台的，在三大操作系统中都是支持的。通过调用该函数可以委托内核帮我们检测若干文件描述符的状态，本质上就是检测这些文件描述符对应的读写缓冲区的状态：

1. 读缓冲区，检测里面是否包含数据，如果有数据则表明对应的文件描述符就绪
2. 写缓冲区，检测写缓冲区是否可以写，如果有容量可以写，缓冲区对应的文件描述符就绪
3. 读写异常，检测读写缓冲区是否有异常，如果有则该缓冲区对应的文件描述符就绪

使用select函数将委托的文件描述符遍历完毕后，满足条件的文件描述符就会通过select函数的三个参数（表示三个集合）传出，然后根据集合中的文件描述符来处理。

------

函数原型如下：

```c
#include <sys/select.h>
int select(int nfds, fd_set *readfds, fd_set *writefds,fd_set *exceptfds, struct timeval * timeout);
```

函数参数详解：

1. nfds参数指定被监听的文件描述符的总数，通常被设置为select监听的所有文件描述符中的最大值+1，因为描述符是从0开始的。内核需要线性遍历集合中的文件描述符

2. readfds：可读的文件描述符的集合，内核检测集合中对应文件描述符的读缓冲区。读集合一般都是需要检测的，这样才知道哪个文件描述符接收到了数据

   writefds：可写的文件描述符的集合，内核检测集合中对应文件描述符的写缓冲区，如果不需要该参数可以穿个NULL进去。

   exceptfds：内核检测文件描述符是否有异常状态，不需要改参数可以传NULL

   fd_set结构体仅包含一个整型数组，数组中的每个元素的每一位（bit）标记了一个文件描述符。fd_set能够容纳的文件描述符数量由FD_SETSIZE执行，因此这样就限制了select能同时处理的文件描述符的总量

   由于位操作比较频繁，因此使用下面的宏来访问fd_set结构体中的位：

   ```c
   // 将文件描述符fd从set集合中删除 == 将fd对应的标志位设置为0        
   void FD_CLR(int fd, fd_set *set);
   // 判断文件描述符fd是否在set集合中 == 读一下fd对应的标志位到底是0还是1
   int  FD_ISSET(int fd, fd_set *set);
   // 将文件描述符fd添加到set集合中 == 将fd对应的标志位设置为1
   void FD_SET(int fd, fd_set *set);
   // 将set集合中, 所有文件文件描述符对应的标志位设置为0, 集合中没有添加任何文件描述符
   void FD_ZERO(fd_set *set);
   ```

3. timeout：设置select函数的超时时间。他是一个timeval结构类型的指针，采用指针是因为内核会修改该参数然后通知应用程序select等待了多久。不能完全信任该值因为调用失败后返回的值是不确定的

   ```c++
   #include <sys/select.h>
   struct timeval {
       time_t      tv_sec;         /* seconds */
       suseconds_t tv_usec;        /* microseconds */
   };
   ```

------

> fd_set这个文件描述符集合的大小是128字节，也就是有1024个标志位，和内核中文件描述符表中的文件描述符个数一样的，这不是巧合，因为fd_set中的每一个bit和文件描述符表中的每一个文件描述符是一一对应的关系

下图是fd_set委托内核检测读缓冲区文件描述符集合

<img src="https://s2.loli.net/2022/01/12/c7aBeo9ru4wPNq5.png" alt="img" style="zoom:25%;" />

内核在遍历读集合的过程中，如果被检测的文件描述符对应的读缓冲区没有数据，内核将修改文件描述符在fd_set的标志位，改为0，如果有数据则为1

### epoll（Linux）

epoll使用一组函数来完成任务，而不是单个函数。

epoll把用户关心的文件描述符上的事件放在内核里的一个事件表中，从而不用像select那样每次调用都需要重复传入文件描述符集合。

但是epoll需要一个额外的文件描述符来唯一标识内核中的事件表，用`epoll_create`函数创建。

------

epoll中一共有三个函数，分别处理不同的操作：

```c
#include <sys/epoll.h>
// 创建epoll实例，通过一棵红黑树管理待检测集合
int epoll_create(int size);
// 管理红黑树上的文件描述符(添加、修改、删除)
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
// 检测epoll树中是否有就绪的文件描述符
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

struct epoll_event {
	uint32_t     events;      /* Epoll 事件 */
	epoll_data_t data;        /* 用户数据 */
};

// 联合体, 多个变量共用同一块内存        
typedef union epoll_data {
 	void        *ptr;
	int          fd;	// 通常情况下使用这个成员, 和epoll_ctl的第三个参数相同即可
	uint32_t     u32;
	uint64_t     u64;
} epoll_data_t;
```

1. `int epoll_create(int size);`

   size参数不起作用，只是提示内核事件表需要多大，大于0即可

   失败返回-1

   成功返回一个有效的文件描述符，通过这个文件描述符就可以访问创建的epoll实例了

2. `int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)`

   - epfd就是epoll_create()函数的返回值，找到相应的epoll实例
   - op，是一个枚举值，指定函数的操作类型，有如下几个
     - `EPOLL_CTL_ADD`:往 epoll 模型中添加新的节点
     - `EPOLL_CTL_MOD`：修改 epoll 模型中已经存在的节点
     - `EPOLL_CTL_DEL`：删除 epoll 模型中的指定的节点
   - fd是文件描述符，即要进行op操作的文件描述符
   - event，epoll事件，用来修饰第三个参数对应的文件描述符，检测这个文件描述符是什么事件
     - `EPOLLIN`：读事件，接收数据，检测读缓冲区，如果有数据该文件描述符就绪
     - `EPOLLOUT`：写事件，发送数据，检测写缓冲区，如果可写该文件描述符就绪
     - `EPOLLERR`：异常事件
     - data：用户数据变量，这是一个联合体类型，通常情况下使用里边的 fd 成员，用于存储待检测的文件描述符的值，在调用 epoll_wait() 函数的时候这个值会被传出。

3. `int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)`

   检测创建的 epoll 实例中有没有就绪的文件描述符。

   - epfd，epoll_create () 函数的返回值，通过这个参数找到 epoll 实例
   - events，传出参数，这是一个结构体数组的地址，里边存储了已就绪的文件描述符的信息
   - maxevents，修饰第二个参数，结构体数组的容量（元素个数）
   - timeout，如果检测的 epoll 实例中没有已就绪的文件描述符，该函数阻塞的时长，单位 ms 毫秒

------

**epoll工作模式：**

- 水平模式LT（level triggered）

  是缺省的工作方式，并且同时支持block和no-block socket。在这种做法中，内核通知使用者哪些文件描述符已经就绪，之后就可以对这些已就绪的文件描述符进行 IO 操作了。**如果我们不作任何操作，内核还是会继续通知使用者。**

  特点如下：

  1. 读事件。如果文件描述符对应的读缓冲区还有数据，读事件就会被触发，epoll_wait () 解除阻塞后，就可以接收数据了。如果接收数据的 buf 很小，不能全部将缓冲区数据读出，那么读事件会继续被触发，直到数据被全部读出，如果接收数据的内存相对较大，读数据的效率也会相对较高（减少了读数据的次数）。**因为读数据是被动的，必须要通过读事件才能知道有数据到达了，因此对于读事件的检测是必须的**
  2. 写事件。如果文件描述符对应的写缓冲区可写，写事件就会被触发，epoll_wait () 解除阻塞后，之后就可以将数据写入到写缓冲区了，写事件的触发发生在写数据之前而不是之后，被写入到写缓冲区中的数据是由内核自动发送出去的
     如果写缓冲区没有被写满，写事件会一直被触发
     因为写数据是主动的，并且写缓冲区一般情况下都是可写的（缓冲区不满），因此对于写事件的检测不是必须的

- 边沿模式ET（edge-triggered）

  是高速工作方式，只支持no-block socket。在这种模式下，当文件描述符从未就绪变为就绪时，内核会通过epoll通知使用者。然后它会假设使用者知道文件描述符已经就绪，并且不会再为那个文件描述符发送更多的就绪通知（only once）。如果我们对这个文件描述符做 IO 操作，从而导致它再次变成未就绪，当这个未就绪的文件描述符再次变成就绪状态，内核会再次进行通知，并且还是只通知一次。ET模式在很大程度上减少了epoll事件被重复触发的次数，因此效率要比LT模式高。

  特点如下：

  1. 读事件。当读缓冲区有新的数据进入，读事件被触发一次，没有新数据不会触发该事件，如果有新数据进入到读缓冲区，读事件被触发，epoll_wait () 解除阻塞，读事件被触发，可以通过调用 read ()/recv () 函数将缓冲区数据读出，如果数据没有被全部读走，并且没有新数据进入，读事件不会再次触发，只通知一次，如果数据被全部读走或者只读走一部分，此时有新数据进入，读事件被触发，并且只通知一次
  2. 写事件。当写缓冲区状态可写，写事件只会触发一次,如果写缓冲区被检测到可写，写事件被触发，epoll_wait () 解除阻塞，写事件被触发，就可以通过调用 write ()/send () 函数，将数据写入到写缓冲区中，写缓冲区从不满到被写满，期间写事件只会被触发一次，写缓冲区从满到不满，状态变为可写，写事件只会被触发一次.

  **epoll 的边沿模式下 epoll_wait () 检测到文件描述符有新事件才会通知，如果不是新的事件就不通知，通知的次数比水平模式少，效率比水平模式要高。**

------

**epoll 在边沿模式下，必须要将套接字设置为非阻塞模式**

对于写事件的触发一般情况下是不需要进行检测的，因为写缓冲区大部分情况下都是有足够的空间可以进行数据的写入。对于读事件的触发就必须要检测了，因为服务器也不知道客户端什么时候发送数据，如果使用 epoll 的边沿模式进行读事件的检测，有新数据达到只会通知一次，那么必须要保证得到通知后将数据全部从读缓冲区中读出。那么，应该如何读这些数据呢？

1. 准备一块特别大的内存，用于存储从读缓冲区中读出的数据，但是这种方式有很大的弊端：首先内存的大小没有办法界定，太大浪费内存，太小又不够用。其次系统能够分配的最大堆内存也是有上限的，栈内存就更不必多言了

2. 循环接收数据。

   ```c
   int len = 0;
   while((len = recv(curfd, buf, sizeof(buf), 0)) > 0)
   {
       // 数据处理...
   }
   ```

   这样做也是有弊端的，因为套接字操作默认是阻塞的，当读缓冲区数据被读完之后，读操作就阻塞了也就是调用的 read()/recv() 函数被阻塞了，当前进程 / 线程被阻塞之后就无法处理其他操作了。

   要解决阻塞问题，就需要将套接字默认的阻塞行为修改为非阻塞，需要使用 fcntl() 函数进行处理

> 通过上述分析就可以得出一个结论：epoll 在边沿模式下，必须要将套接字设置为非阻塞模式，但是，这样就会引发另外的一个 bug，在非阻塞模式下，循环地将读缓冲区数据读到本地内存中，当缓冲区数据被读完了，调用的 read()/recv() 函数还会继续从缓冲区中读数据，此时函数调用就失败了，返回 - 1，对应的全局变量 errno 值为 EAGAIN 或者 EWOULDBLOCK 如果打印错误信息会得到如下的信息：Resource temporarily unavailable

## 信号

> 信号是由用户，系统或者进程发送给目标进程的信息，用来通知目标进程某个状态的改变或系统异常。Linux信号有以下条件产生：
>
> 1. 对于前台进程，通过用户输入的某些字符产生信号，比如`Ctrl+C`
> 2. 系统异常，比如除以零
> 3. 系统状态变化。比如alarm定时器到期引起SIGALRM信号
> 4. 运行kill命令或者调用kill函数。

### 发送信号

Linux下一个进程给其他进程发送信号的API是kill函数

```c
#include <sys/types.h>
#include <signal.h>

// sig：信号；pid：目标进程
// 该函数成功时返回0，失败时返回-1并设置errno
int kill(pid_t pid, int sig);
```

### 处理信号

目标进程收到信号后，需要定义一个接收函数来处理，信号处理函数原型如下：

```c
#include <signal.h>
typedef void (*__sighandler_t) (int);
```

这个是用户自定义的信号处理函数。除了自定义也可以用系统指定的：

```c
#include <bits/signum.h>
#define SIG_DFL ((__sighandler_t) 0)	//忽略目标信号
#define SIG_IGN ((__sighandler_t) 1) 	//使用信号的默认处理方式
```

信号的默认处理方式主要有：

1. Term：结束进程
2. Ign：忽略信号
3. Core：结束进程并生成核心转储文件
4. Stop：暂停进程
5. Cont：继续进程

### 网络编程相关的Linux信号

- **SIGHUP**

  当挂起进程的控制终端时，SIGHUP信号会被触发，这个信号的默认行为是Term（结束进程）

  对于没有控制终端的网络后台程序而言，他们通常利用SIGHUP信号来强制服务器重读配置文件

  例如xinetd超级服务器程序，在接收到SIGHUP信号之后将调用hard_config函数，循环读取/etc/xinted.d/目录下的每个子配置文件，并检测其变化。如果某个正在运行的子服务的配置文件被修改以停止服务，则xinetd主进程将给该子服务进程发送SIGTERM信号结束他。如果某个子服务的配置文件被修改以开启服务，则xinetd将创建新的socket并且绑定到该服务对应的端口上

- **SIGPIPE**

  默认情况下，往一个读端关闭的管道或socket连接中写数据将引发SIGPIPE信号，我们在代码中应该捕获处理它，或至少忽略它，因为它的默认行为是结束进程，我们肯定不希望错误的写操作导致程序退出。

- **SIGURG**

  Linux环境下，内核通知应用程序带外数据到达主要有两种方法：一种是I/O复用技术，select等系统调用在接收到带外数据时将返回并报告socket上的异常事件，另外一种是使用SIGURG信号。SIGURG的默认行为是Ign。

### 信号函数

我的理解是这个系统调用函数将信号和信号处理函数集合到一起，即调用该信号函数就可以指定信号和该信号对应的信号处理函数。

主要有signal和sigaction两个

```c
#include <signal.h>

// sig：指出要捕获的信号类型，_handler用于指定信号sig的处理函数
// 成功时返回类型为_sighandler_t的函数指针，错误返回SIG_ERR并设置errno
_sighandler_t signal(int sig, _sighandler_t _handler);

// 更健壮的信号处理函数接口
// sig:要捕获的信号类型；act:制定新的信号处理方式；oact：输出信号前的处理方式
int sigaction(int sig, const struct sigaction* act, struct sigaction* oact);

// sigaction结构体定义：
struct sigaction{
#ifdef __USE_POSIX199309
    union{
        _sighandler_t sa_handler;
        void(*sa_sigaction)(int, siginfo_t*, void*);
    }
    _sigaction_handler;
# define sa_handler     __sigaction_handler.sa_handler
# define sa_sigaction   __sigatcion_handler.sa_sigaction
#else
    _sighandler_t sa_handler;
#endif

    _sigset_t sa_mask;
    int sa_flags;
    void(*sa_restorer)(void);
};
```

### 信号集

Linux使用数据结构sigset_t来表示一组信号

```c
#include <bits/sigset.h>
#include <signal.h>

// Linux用数据结构sigset_t来表示一组信号
#define _SIGSET_NWORDS (1024/(8*sizeof(unsigned long int)))
typedef struct{
    unsigned long int __val[_SIGSET_NWORDS];
} __sigset_t;

//提供一组函数来设置、修改、查询、删除信号集
int sigemptyset(sigset_t* _set);                // 清空信号集
int sigfillset(sigset_t* _set);                 // 在信号集中设置所有信号
int sigaddset(sigset_t* _set, int _signo);      // 将信号_signo添加至信号集中
int sigdelset(sigset_t* _set, int _signo);      // 将信号_signo从信号集中剔除
int sigismember(_const sigset_t*, int _signo);  // 测试_signo是否在信号集中
```

## 定时器

定时器在网络程序中也很重要，比如定期检测一个客户的活动状态。

服务器程序通常管理着众多的定时器，因此有效的组织这些定时器，使得他们能在预期的时间点被触发而且不影响服务器的主要逻辑这个特性对服务器的性能有着重要的影响。

因此我们会将定时事件封装成定时器，并使用某种容器类数据结构将所有的定时器集合到一起，比如用链表，时间轮，时间堆，以实现对事件的统一管理。

### 基于生序链表的定时器

### 时间轮

[时间轮实现](../LinuxCode/time_wheel.h)

基于升序链表来管理定时器这种方式不适合定时器过多的场景，因为就一条链表，随着链表上定时器的增多会造成插入效率的降低。

<img src="https://cdn.jsdelivr.net/gh/luogou/cloudimg/data/202202161440014.png" alt="img" style="zoom:67%;" />

上图时间轮的特点：

1. 上图有N个槽，每一个槽叫做slot
2. 指针顺时针转动，从slot1转动到slot2，时间间隔是tick，即心搏时间。因此转完一圈所用的时间是N*tick
3. 每一个slot指向一个定时器链表，每条链表上的定时器有相同的特征
4. 两个slot中的定时器链表时间差是tick间隔时间的整数倍
5. 要将定时时间为ti的定时器插入时间轮中，假如现在指向slot1，则会加入$slot=(slot_1 + (t_i/tick))%N$
6. 对时间轮而言，如果要提高精度，就要使得tick值足够小；要提高执行效率，需要使得N值足够大。
7. 本质思想就是利用哈希表，将定时器散列到不同的链表上，提高效率。

分析：添加一个定时器的时间复杂度是O(1)，删除一个定时器的时间复杂度是O(1)，执行一个定时器的时间复杂度是O(n)。

### 时间堆

时间轮是以固定的频率调用心搏函数tick的，并在其中依次检测到期的定时器，然后执行到期定时器上的回调函数。

但是时间堆是另外一种思路：将所有定时器中超时时间最小的一个定时器的超时值作为心搏间隔，这样的话一旦心搏函数tick被调用，超时时间最小的定时器必然到期，然后就可以在tick函数中处理该定时器。接着再从剩余的找个最小的，将这段最小时间设置为下一次心搏间隔。这样可以发现一个间隔（心搏时间）必然会触发一个定时器，这种定时方法比较精准。

所以**最小堆**很容易实现这种定时方案。

分析：添加一个定时器的时间复杂度是O(lgn)，删除一个定时器的复杂度是O(1)，执行一个定时器的时间复杂度是O(1)，因此效率很高。

## 多进程编程--IPC

### 管道

```C++
int pipe(int pipefd[2]);
//通常在内核中提供4KB大小的缓冲区

int socketpair(int domain, int type, int protocol, int sv[2]);

int mkfifo(const char *pathname, mode_t mode);
//fifo是一种特殊的文件类型，保存在内存中而不是磁盘中
```

### 信号量

[信号量通信实例](../LinuxCode/semaphore.c)

```c++
int semget(key_t key, int num_sems, int sem_flags);

key:      所创建或要获取的信号量集的键。
num_sems: 所创建的信号量集中信号量的个数，此参数只在创建一个新的信号量集时有效。
flag:     调用函数的操作类型，也可以设置信号量集的权限。
```

```c++

unsigned short semval;   //信号量的值
unsigned short semzcnt;  //等待信号量变为0的进程数量
unsigned short semncnt;  //等待信号量增加的进程数量
pid_t sempid;            //最后一次执行semop操作的进程ID

int semop(int sem_id, struct sembuf *set_ops, size_t num_sem_ops); //semop对信号量的操作实际上就是对上面内核变量的操作

semid：信号量集的引用ID
sops: 一个sembuf结构体数组，用于指定调用semop函数所做的操作。
nsops: sops数组元素的个数。

struct setmbuf{
    unsigned short int sem_num;
    short int sem_op;
    short int sem_flg;
};
```

```c++
int semctl(int sem_id, int sem_num, int command, ...);
```

### 共享内存

>最高效的进程通信机制，因为不涉及进程之间的任何数据传输

```c++
int shmget(key_t key, size_t size, int shmflg);

key: 标示一段全局唯一的共享内存
size:指示共享内存的大小

//如果shmget用于共享内存的创建，则这段共享内存所有字节都被初始化为0， 与之关联的内核数据结构shmid_ds将被构建并初始化
```

```c++
void *shmat(int shm_id, const void *shm_addr, int shmflg);
int shmdt(const void *shm_addr);
```

```c++
shmctl(int shm_id, int command, struct shmid_ds *buf);

```

### 消息队列

```c++
int msgget(key_t key, int msgflg);
int msgsnd(int msqid, const void *msg_ptr, size_t msg_sz, int msgflg);

msqid   msgget返回的消息队列标识符
msg_ptr 指向一个准备发送的消息，消息必须被定义为如下类型：
struct msgbuf {
    long mtype;
    char mtext[512];
};

int msgrcv(int msqid, void *msg_ptr, size_t msg_sz, long int msgtype, int msgflg);

msg_ptr 用于接受消息
msg_sz  表示消息数据部分长度
msgtype = 0 读取队列中第一条消息
msgtype > 0 读取队列中第一条类型为msgtype的消息
msgtype < 0 读取队列中第一条类型值比msgtype绝对值小的消息

int msgctl(int msqid, int command, struct msqid_ds *buf);

```

## 多线程编程

### 创建线程和结束线程

```c++
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg);

thread 线程的标示符
attr   设置线程的属性，NULL表示使用默认线程属性
start_roution 和 arg 表示即将运行的函数以及函数的参数

void pthread_exit(void *retval);
int pthread_join(pthread_t thread, void **reval);
int pthread_cancel(pthread_t thread);
```

### 线程属性

线程属性全部包含在一个字符数组中。线程库定义了一些列函数来操作 pthread_attr_t 类型的变量

```C++
#defien __SIZEOF_PTHREAD_ATTR_T 36
typedef union{
    char __size[__SIZEOF_PTHREAD_ATTR_T];
    long int __align;
}pthread_attr_t;
```

### 线程同步方式

[线程同步封装](../LinuxCode/lock.h)

#### POSIX信号量

```c++
int sem_init(sem_t *sem, int pshared, unsigned int value);
//初始化一个未命名的信号量，其中pshare表示信号量是当前进程的局部信号量还是可以在多个进程之间共享。
int sem_destroy(sem_t *sem);
//销毁信号量，释放其占用的内核资源，如果销毁一个被其他线程等待的信号量，会产生不会预估的后果。
int sem_wait(sem_t *sem);
//sem_wait以原子操作将信号量的值减一，若信号量值为0，则该函数被阻塞。
int sem_trywait(sem_t *sem);
//sem_trywait每次都立刻返回，不论被操作的信号量是否为0，相当于非阻塞
int sem_post(sem_t *sem);
//以原子方式将信号量值加1，当信号量的值大于0时，其他调用sem_wait等待的线程会被唤醒，这些等待线程在等待队列中。
```

#### 互斥锁

```c++
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
```

#### 条件变量

```c++
int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *cond_attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
```

## 进程池和线程池
