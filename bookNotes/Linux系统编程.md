# Linux 系统编程

## 文件IO

* 文件名与`inode`节点的关联被称作链接
* 内核为每个进程维护一个打开文件表`（file table）`，由文件描述符索引。打开文件表存储了指向了文件inode的内存拷贝的指针和元数据（例如文件位置、访问模式等）
* 子进程默认获得一份父进程文件表的拷贝
* 每个进程按照惯例会至少有三个打开的文件描述符：0，1，2，除非进程显式地关闭他们

### `open`

```C++
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int open (const char *name, int flags);
int open (const char *name, int flags, mode_t mode);

O_APPEND    追加模式打开
O_ASYNC     指定文件可写或者可读时产生一个信号（默认是SIGIO）该信号仅用于套接字和终端
O_DIRECT    打开文件用于直接IO
O_NONBLOCK  非阻塞模式下打开
O_SYNC      用于同步IO （可以理解为在每个write操作之后都隐式地调用fsync，但是由内核实现，效率更高）
O_TRUNC     将文件长度截断为0
```

### `creat`

```C++
int creat(const char *name, mode_t mode);

// => open(file, O_TRUNC | O_WRONLY | O_CREAT, mode)
```

### `read`

```C++
sszize_t read(int fd, void *buf, size_t len);

调用返回-1，并且errno被设置为EINTR，表示读入字节之前收到了一个信号，可以重新进行调用
调用返回-1，并且errno被设置为EAGAIN，表示没有可用的数据，读请求应该在之后重开，只在非阻塞模式下发生
```

### `wirte`

```C++
ssize_t write(int fd, const void *buf, size_t count);
```

### `fsync`

```C++
int fsync(int fd);
//保证fd对应文件的脏数据回写到磁盘上
int fdatasync(int fd);
//仅保证数据写回，不保证元数据
```

### `sync`

```C++
void sync(void);
//没有参数和返回值，总是成功返回，确保所有的缓冲区都能写入磁盘
```

### `close`

```C++
int close(int fd);
```

### `lseek`

```C++
off_t lseek(int fd, off_t pos, int origin);
SEEK_CUR 当前位置
SEEK_END 文件末尾
SEEK_SET 设置为pos
```

### `pread`

```C++
ssize_t pread(int fd, void *buf, size_t count, off_t pos);
ssize_t pwrite(int fd, const void *buf, size_t count, off_t pos);
//使用更加便捷、不会修改文件位置指针
```

### `ftruncate`

```C++
int ftruncate(int fd, off_t len);
int truncate(const char *path, off_t len);

//成功返回0
```

### `select`

```C++
int select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
FD_CLR(int fd, fd_set *set);
FD_ISSET(int fd, fd_set *set);
FD_SET(int fd, fd_set *set);
FD_ZERO(int fd, fd_set *set);

/*
readfds   可读数据
writefds  可写数据
exceptfds 异常发生或者带外数据
n         所有集合中文件描述符最大值+1

timeout   即使没有文件描述符处于IO就绪状态，select也将在timeout时间后返回

返回就绪描述符的个数
*/
```

### `poll`

```C++
int poll(struct pollfd *fds, unsigned int nfds, int timeout);

struct poolfd {
    int fd;
    short events;
    short revents;
};

POLLIN     =》POLLRDNORM ｜ POLLRDBAND
POLLRDNORM 有正常数据可读
POLLRDBAND 有优先数据可读
POLLPRI    有高优先级数据可读
POLLOUT    =》POLLWRNORM
POLLWRNORM 写正常数据不会阻塞
POLLBAND   写优先数据buhuizuse 
POLLMSG    有一个SIGPOLL消息可用
POLLER     fd有错误
POLLHUP    文件描述符上有挂起事件
POLLNVAL   给出的fd非法

```

## 缓冲输入输出

## 高级文件IO

### `readv`

```C++
ssize_t readv(int fd, const struct iovec *iov, int count);
ssize_t writev(int fd, const struct iovec *iov, int count);

struct iovec{
    void *iov_base;
    size_t iov_len;
};
```

### `epoll`

```C++
int epoll_create(int size);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
```

### `mmap`

```C++
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t len);

PROT_READ
PROT_WRITE
PROT_EXEC

MPA_FIXED    将addr看作是强制性要求
MAP_PRIVATE  映射区不共享
MAP_SHARED   和所有其他映射文件的进程共享映射内存
```

## 进程管理

### `exec`

```C++
int execlp(const char *file, const char *arg, ...);
int execle(const char *path, const char *arg, ..., char *const envp[]);
int execv(const char *path, char *const argv[]);
...
```

### `fork`

```C++
pid_t fork(void);
pid_t vfork(void);
```

### `exit`

```C++
void exit(int status);
```

### `wait`

```C++
pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);
pid < -1 : 等待该进程组所有的子进程
pid = -1 : 等待任意子进程
pid = 0  : 等待同组任意进程
```

## 高级进程管理

## 文件与目录管理

### `stat`

```C++
int stat(const char *path, struct stat *buf);
int fstat(int fd, struct stat *buf);
int lstat(const char *path, struct stat *buf);
```

### `chmode`

```C++
int chmode(const char *path, mode_t mode);
int fmode(int fd, mode_t mode);
```

### 工作目录

```C++
char *getcwd(char *buf, size_t size);
int chdir(const char *path);
int fchdir(int fd);
int mkdir(const char *path, mode_t mode);
int rmdir(const char *path);
DIR *opendir(const char *name);
struct dirent *readdir(DIR *dir);
int closedir(DIR *dir);
```

### 链接

```C++
int link(const char *oldpath, const char *newpath);
int symlink(const char *oldpath, const char *newpath);
int unlink(const char *pathname);
int remove(const char *path);
```

## 内存管理

### `malloc`

```C++
void *malloc(size_t size);
void free(void *ptr);
```

## 信号

### `signal`

```C++
typedef void (*sighandler_t)(int);
sighandler_t signal(int signo, sighandler_t handler);
extern const char *const sys_siglist[]; 
//进程不能捕获SIGKILL和SIGSTOP
```

### `pause`

```C++
int pause(void);
```

### `kill`

```C++
int kill(pid_t pid, int signo);
```

### `raise`

```C++
int raise(int signo);
```

### 信号集

```C++
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signo);
int sigdelset(sigset_t *set, int signo);
int sigismember(const sigset_t *set, int signo);
```

### `sigprocmask`

```C++
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);

SIG_SETMASK     
SIG_BLOCK      
SIG_UNBLOCK
```

### `sigpending`

```C++
int sigpending(sigset_t *set);
//获取等待被处理的信号
```
