# Linux多线程服务端编程

* 程序在本机测试正常，放到网络上运行就经常出现数据接受不全的情况
* TCP协议真的有所谓的“粘包问题”吗？
* 该如何设计消息帧的协议？又该如何编码实现分包才不会掉到陷阱里？
* 带外数据（OOB）、信号驱动IO这些高级特性到底有没有用？
* 网络协议格式该怎么设计？发送C struct会有对齐方面的问题吗？
* 对方不用C/C++怎么通信？将来服务端软件升级，需要在协议中增加一
个字段，现有的客户端就必须强制升级？
* 传闻服务端开发主要通过日志来查错，那么日志里该写些什么？
日志是写给谁看的？怎样写日志才不会影响性能？
* 分布式系统跟单机多进程到底有什么本质区别？心跳协议为什么
是必需的，该如何实现？
* C++的大型工程该如何管理？库的接口如何设计才能保证升级的时
候不破坏二进制兼容性？有没有更适合大规模分布式系统的部署方案？

    ...

---

## 背景知识

* RAII
  
    RAII（Resource Acquisition Is Initialization）是C++中一种重要的资源管理技术，它通过构造函数在初始化对象时获取资源，并在析构函数中释放资源，从而避免了手动管理资源的复杂性和容易出错的问题。特别的，对于互斥锁等同步原语的使用，RAII可以确保多线程环境下资源的安全获取和释放，避免了竞态条件和死锁等问题。
* lock_guard

    `std::lock_guard`是一个RAII类型，可以保证锁的自动释放。它的作用类似于`st::unique_lock`，但使用起来更加简单。

    ```c++
    #include <mutex>
    #include <thread>

    std::mutex mtx;             // 互斥锁

    void function() {
        std::lock_guard<std::mutex> lock(mtx);  // 获得互斥锁
        // 访问共享变量
        // ...
    }   // lock自动释放
    ```

    **内部实现**

    ```c++
    template<class Mutex>
    class lock_guard {
    public:
        using mutex_type = Mutex;

        explicit lock_guard(mutex_type& m) : _mutex(m)
        { _mutex.lock(); }

        lock_guard(mutex_type& m, std::adopt_lock_t) noexcept
            : _mutex(m) {}

        ~lock_guard() noexcept
        { _mutex.unlock(); }

        lock_guard(const lock_guard&) = delete;
        lock_guard& operator=(const lock_guard&) = delete;
    private:
        mutex_type& _mutex;
    };    
    ```

* unique_lock
* shared_mutex

## C++多线程系统编程

### 线程安全的对象生命期管理

* 一个线程安全的类
* C++中可能出现的内存问题：
  * 缓冲区溢出
  * 空悬指针
  * 内存泄漏
  * 内存碎片
