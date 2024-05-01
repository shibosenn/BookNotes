# APUE

## 0. 前置知识

- ``IS_ERR`` ``PRR_ERR`` ``ERR_PTR``

    内核空间的最后一个页专门为错误码保留，即内核用最后一页捕捉错误，因此一般人不会用到指向内核空间最后一页的指针

- ``likely`` ``unlikely``

    ```c++
    #define likely(x) __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
    // 可以优化程序编译后的指令序列，从而优化流水线的效率
    ```

- ``BUG_ON ...``

    ```c++
    BUG() // 无条件触发
    BUG_ON(condiction)
    WARN_ON(condiction)
    ...
    ```

- ``restrict``

    用于告知编译器一个指针式访问数据对象的唯一且初始的方式，这能够使得编译器假设指针所指向的数据不会被其他指针所改变，从而带来更好的编译优化效果

## 3. 文件 I/O

### ``open`` ``openat``

```c++
int open(const char *path, int flag, ... /* mode_t mode*/);
int openat(int fd, const char *path, int flag, ... /* mode_t mode*/);
// 当路径为相对路径的时候，打开的是相对于fd所指向目录的路径

int create(const char *path,, mode_t);

// ... -> do_sys_open -> get_unused_fd_flags -> alloc_fd(0, (flags)) -> do_filp_open -> fd_install
// 在alloc_fd 的过程中会修改fdtable->close_on_exec
// do_filp_open 会记录其他flags的信息
```

### ``close``

```c++
int close(int fd);
```

### ``lseek``

```c++
off_t lseek(int fd, off_t offset, int whence); // SEEK_SET -> 0, SEEK_CUR -> 1, SEEK_END -> 2

off_t offset = lseek(fd, 0, 1); // 获取当前文件偏移量

off_t ret = lseek(fd, size, 0); // 创建一个size大小的空洞文件
```

### ``read`` ``write``

```c++
sszie_t read(int fd, void *buf, size_t nbytes);
ssize_t write(int fd, const void *buf, size_t nbytes);
```

### 原子操作

```c++
sszie_t pread(int fd, void *buf, size_t nbytes, off_t offset);
size_t pwrite(int fd, const void *buf, size_t nbytes, off_t offset);

// 将寻位和读写封装成原子操作，O_APPEND也可以实现同样的效果
// pread不从文件表获取当前偏移，直接使用用户传递的偏移，不会更改当前文件的偏移量

/*
    关于append能够实现原子的读写:                
        write -> vfs_write(file, buf, count, &pos) -> do_sync_write -> file->f_op->aio_write
        Linux 大多数文件系统将 aio_write 绑定到 generic_file_aio_write

        generic_file_aio_write中的部分实现: （通过加锁对inode进行了保护） 
            mutex_lock(&inode->i_mutex);
            ret = __generic_file_aio_write(iocb, iov, nr_segs, &iocb->ki_pos);
            mutex_unlock(&inode->i_mutex);
        
        __generic_file_aio_write -> generic_write_check:
            if (file->f_flags & O_APPEND)
                *pos = i_size_read(inode);
        
        可以看到对于表示append的文件，会直接读取文件的大小，进行操作，因此lseek不会对append表示的文件产生任何的影戏那个
*/
```

### ``dup`` ``dup2``

```c++
int dup(int fd);
int dup2(int fd, int fd2);

// 默认不会保留CLOEXEC标识
// 根本原因在于CLOEXEC标识的记录方式和其他flag不同，其他flag直接会保存到 struct file -> flags 中
// 标识CLOEXEC的文件会通过 fdtable->close_on_exec 保存

// 但是dup的调用逻辑 get_unused_fd(0, 0) -> alloc_fd -> fd_install
// 这里使用的get_unused_fd直接使用了0作为flag，所以会导致dup出来的fd没有办法继承CLOEXEC
```

### ``sync`` ``fsync`` ``fdatasync``

```c++
int fsync(int fd);      // 等待写操作结束才返回
int fdatasync(int fd);  // 只影响文件的数据部分
void sync(void);        // 将所有修改过的块缓冲区排入写队列，不等待实际的写磁盘操作结束
```

### ``fcntl``

```c++
int fcntl(int fd, int cmd, .../* int arg */);

/*
复制一个已有的描述符    F_DUPFD F_DUPFD_CLOEXEC
获取/设置文件描述符     F_GETFD F_SETFD --> FD_CLOEXEC ( 1 for set )
获取/设置文件状态标志   F_GETFL F_SETFL 在修改时候要谨慎，应该在当前的flag的基础上进行修改
获取/设置异步I/O所有权
获取/设置记录锁
*/
```

## 4. 文件和目录

### ``stat`` ``fstat`` ``fsatat`` `lstat`

```c++
int stat(const char *restrict pathname, struct stat *restrict buf);
int fstat(int fd, struct stat *buf);
int lstat(const char *restrict pathname, struct stat *restrict buf); // 返回符号链接本身，而不是引用文件的信息
int fstat(int fd, const char *restrict pathname, struct stat *restrict buf, int flag); 

// stat -> vfs_stat -> vfs_getattr -> generic_fillattr
// 通过 st_mode 可以查看文件类型和文件权限
```

### 文件类型

- 普通文件
- 目录文件
- 块特殊文件
- 字符特殊文件
- 命名管道
- 套接字
- 符号链接

### 文件访问权限

- 用名字打开任一类型的文件时，对该名字中包含的每一个目录，都应该具有执行权限，目录的执行权限位常常被称为搜索位
- 在目录中创建/删除文件，必须要有对目录的写权限和搜索权限

### `access` 和 ``faccessat``

```c++
int access(const char *pathname, int mode); // mode: R_OK W_OK X_OK
int faccess(int fd, const char *pathname, int mode); // pathname为绝对路径，或者fd取值为AT_FDCWD效果相同
```

### ``umask``

```c++
mode_t umask(mode_t umask);
// 返回原始的权限
```

### ``chmod`` ``fchowd`` ``fchownad``

```c++
int chmod(const char *pathname, mode_t mode);
int fchmod(int fd, mode_t mode);
int fchmodat(int fd, const char *pathname, mode_t mode, int flag);  // flag可以用来控制是否跟随符号链 
                                                                    // AT_SYMLINK_NOFOLLOW
```

### ``chown`` ``fchown`` ``fchownat`` ``lchown``

```c++
int chown(const char *pathname, uid_t owner, gid_t group);
int fchown(int fd, uid_t owner, gid_t group);
int fchownat(int fd, const char *pathname, uid_t owner, gid_t group, int flag);
int lchown(const char *pathname, uid_t owner, gid_t group);
```

### 文件截断

```c++
int truncate(const char *pathname, off_t length);
int ftruncate(int fd, off_t length);
```

### ``link`` `linkat` ``unlink`` ``unlinkat`` ``remove``

```c++
int link(const char *existingpath, const char *newpath);
int linkat(int efd, const char *existingpath, int nfd, const char *newpath, int flag);
int unlink(const char *pathname);
int unlinkat(int fd, const char *pathname, int flag); // 可以通过设置为AT_REMOVEDIR来达到删除目录的效果
int remove(const char *pathname) // 对于文件，功能和unlink相同，对于目录，功能和rmdir相同
```

### ``rename`` ``renameat``

```c++
int rename(const char *oldname, const char *newname);
int renameat(int oldfd, const char *oldname, int newfd, const char *newname);
```

### 创建和读取符号链接

```c++
int symlink(const char *actualpath, const char *sympath);
int symlinkat(const char *actualpath, int fd, const char *sympath);

size_t readlink(const char *restrict pathname, char *restrict buf, size_t bufsize);
size_t readlinkad(int fd, const char *restrict pathname, char *restrict buf, size_t bufsize);
```

### ``mkdir`` ``mkdirat`` ``rmdir``

```c++
int mkdir(const char *pathname, mode_t mode);
int mkdirat(int fd, const char *pathname, mode_t mode);
int rmdir(const char *pathname);
```

### 读目录

```c++
DIR *opendir(const char *pathname);
DIR *fdopendir(int fd);
struct dirent *readdir(DIR *dp);
void rewinddir(DIR *dp);
int closedir(DIR *dp);
long telldir(DIR *dp);
void seekdir(DIR *dp, long loc);
```

### ``chdir`` ``fchdir`` `getcwd`

```c++
int chdir(const char *pathname);
int fchdir(int fd);
char *getcwd(char *buf, size_t size);
```

## 5. 标准 IO 库

### 流 和 FILE 对象

```c++
int fwide(FILE *fp, int mode);
```

### 缓冲

```c++
void setbuf(FILE *restrict fp, char *restrict buf);
int setvbuf(FILE *restrict fp, char *restrict buf, int mode, size_t size); // mode : _IOFBF _IOLBF _IONBF

int fflush(FILE *fp);
```

### 打开流

```c++
FILE *fopen(const char *restrict pathname, const char *restrict type);
FILE *freopen(const char *restrict pathname, const char *restrict type, FILE *restrict fp);
// 在一个指定的流上打开指定的文件，如果流已经打开，则先关闭流；
FILE *fdopen(int fd, const char *type);
```

![fopen type参数](./imags/fopenType参数.png)

除非终端设备，不然默认全缓冲

### 读和写流

```c++
int getc(FILE *fp);         // 可能被实现为宏
int fgetc(FILE *fp);        // 一定是一个函数
int getchar(void);          // getc(stdin)

// 由于出错和EOF的返回值是一样的，为了区分这两种情况，需要调用
int ferror(FILE *fp);
int feof(FILE *fp);
void clearerr(FILE *fp);

int ungetc(int c, FILE *fp);
```

```c++
int putc(int c, FILE *fp);
int fputc(int c, FILE *fp);
int putchar(int c);
```

### 每次一行 IO

```c++
char *fgets(char *restrict buf, int n, FILE *restrict fp);
char *gets(char *buf);

int fputs(const char *restrict str, FILE *restrict fp);
int puts(const char *str);
```

### 二进制 IO

```c++
size_t fread(void *restrict ptr, size_t size, size_t nobj, FILE *restrict fp);
size_t fwrite(const void *restrict ptr, size_t size, size_t nobj, FILE *restrict fp);
// 只能在同一系统，原因有二：
// 同一个结构，不同的系统，可能会有不同的布局
// 不同系统的浮点数二进制格式可能也不相同
```

### 定位流

```c++
long ftell(FILE *fp);
int fseek(FILE *fp, long offset, int whence);
void rewind(FILE *fp);

off_t ftello(FILE *fp);
int fseeko(FILE *fp, off_t offset, int whence);

int fgetpos(FILE *restrict fp, fpos_t *restrict pos);
int fsetpos(FILE *fp, const fpos_t *pos);
```

### 格式化 IO

```c++
int printf(const char *restrict format, ...);
int fprintf(FILE *restrict fp, const char *restrict format, ...);
int dprintf(int fd, const char *restrict format, ...);
int sprintf(char *restrict buf, const char *restrict format, ...);
int snprintf(char *restrict buf, size_t n, const char *restrict format, ...);

int scanf(const char *restrict format, ...);
int fscanf(FILE *restrict fp, const char *restrict format, ...);
int sscanf(const char *restrict buf, const char *restrict format, ...);

```

### 实现细节

```c++
int fileno(FILE *fp);
```

### 临时文件

```c++
char *tmpnam(char *ptr);  // 多次调用会冲刷内部的静态缓冲区，因此要及时保存，不是原子操作，会存在竞态条件
FILE *tmpfile(void);    
```

### 内存流

```c++
FILE *fmemopen(void *restrict buf, size_t size, const char *restrict type);
```

## 进程环境

### 命令行参数

### 环境表

### C程序的存储布局空间

### 共享库

### 存储空间分配

### 环境变量

## 进程控制

## 进程关系

## 信号

## 线程

## 线程控制

## 守护进程

## 高级 IO

## 进程间通信

## 网路IPC

## 高级IPC

## 终端IO

```c++
```

```c++
```

```c++
```

```c++
```

```c++
```

```c++
```

```c++
```

```c++
```

```c++
```

```c++
```
