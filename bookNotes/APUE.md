# APUE

## 0. 前置知识

- **POSIX** **BSD** **SystemV**

    System V 和 BSD 原本都是 UNIX 系统的不同分支，它们各自提出了不同的特性和改进，对后续的 UNIX 和类 UNIX 系统产生了深远影响。
POSIX 作为一个标准，试图融合不同 UNIX 系统（包括 System V 和 BSD）的特性，以确保软件在不同系统之间的可移植性。许多从 System V 和 BSD 派生的现代操作系统都遵循 POSIX 标准。

    System V 和 BSD 是具体的操作系统实现，它们具有各自独特的功能和工具；POSIX 是跨平台的标准集，定义了应该如何实现这些功能以确保跨平台兼容性。

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

- ``offsetof`` ``coniner_of``

    ```c++
    #define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER) // 把0地址强制转化为type类型，然后取成员地址，转化成为偏移即可
    #define container_of(ptr, type, member) ({          \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type, member) );})

    ```

- ``__user`` ``__kernel`` ``__iomem``

    保证指针空间必须在用户/内核/设备地址空间

- ``LDREX`` ``STREX``

    ```c++
    /*
    LDREX : Load Register Exclusive
    STREX : Store Register Exclusive


    ldrex 指令从内存中加载一个值到寄存器，并标记该内存地址开始一个独占访问。这意味着该指令告诉处理器，接下来对这个地址的操作希望是独占的，即在本次 ldrex 操作和接下来的 strex 操作完成之前，不希望其他处理器对这个地址进行写操作。

    strex 指令尝试将一个寄存器的值存储到先前由 ldrex 指令标记为独占的内存地址。如果自 ldrex 执行以来没有其他处理器写入该内存地址，strex 将写入成功，并返回 0；如果有其他处理器介入修改了该地址，则 strex 写入失败，返回非零值。

    STERXPL STREXEQ 是拓展的条件执行版本，允许在满足特定条件的情况下才执行存储操作
    */
    ```

- 内嵌汇编指令

    ```c++
    // 基本语法如下所示：
    asm volatile (assembly code : output operands : input operands : clobbered registers); // volatile 可选，告诉编译器不要优化这段编译代码

    // 操作数约束 r(寄存器) m(内存) g(编译器选择最合适的方式) i(立即数) ...
    // 特殊约束操作符 =(约束的是一个输出操作数) &(通常和=搭配使用，表示不能与任何输入操作数共享寄存器)
    // 按顺序使用 %0， %1 ... 引用操作数
    ```

- ``cpu_relax``

    - 降低能耗

    - 在超线程环境中，减少核心争用，更多的将资源让渡给其他线程

- 可重入函数

    - 尽量使用局部变量
    - 使用全局变量要加以保护

- **ACCESS_ONCE**

    ```c++
    #define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))

    // 可以防止一些编译器错误优化的发生

    while(should_continue)  // ---> while(ACCESS_ONCE(should_continue))
        do_something();

    // 如果do_something完全没有任何修改，完全可以优化成
    if(should_continue)
        for(;;)
            do_something();
    
    p = global_ptr         // ---> p = ACCESS_ONCE(global_ptr)
    if(p && p->s && p->s>func) 
        ...

    // 也许会优化成
    if(global_ptr && globale_ptr->s && global_ptr->s->func)
        ...
    ```

- ``preempt_disable``

    ```c++
    # define add_preempt_count(val)	do { preempt_count() += (val); } while (0)

    #define inc_preempt_count() add_preempt_count(1)
    #define dec_preempt_count() sub_preempt_count(1)

    #define preempt_count()	(current_thread_info()->preempt_count)

    #define preempt_disable() \
    do { \
        inc_preempt_count(); \
        barrier(); \
    } while (0)
    ```

## 00. Linux 中常见的数据结构

### 基础数据结构

- 链表

    >如果链表不能包容万物，就让万物包容链表

    ```c++
    struct list_head {
        struct list_head *next, *prev;
    };
    ```

- 哈希表

    ```c++
    struct hlist_head {
        struct hlist_node *first;
    };

    struct hlist_node {
        struct hlist_node *next, **pprev;
    };

    // 通过这种设计，可以优化内存使用
    ```

### 并发控制

- ``atomic_t``

    ```c++
    typedef struct {
        volatile int counter;   // 告诉编译器，counter可能被程序外部修改，比如其他线程、中断服务例程
    } atomic_t;
    // volatile 关键字保证编译器不在进行优化，系统总是重新从他所在的内存读取数据
    ```

    单处理器系统中，处理器的执行流程只会收到中断机制的影响，因此可以通过 “提供能完成多步操作的单条指令” 或者 ”关中断“ 的方式实现原子操作

    多处理器系统中，即使是一条指令执行的期间也会收到其他干扰，不同架构提供了不同的实现原子操作的方式，比如 **X86** 架构可以通过对总线加锁保证只允许一个处理器访问， **ARM** 可以通过独占内存实现， RISC-V 可以通过 **CAS** 实现原子操作

    ```c++
    // X86 架构实现
    static inline void atomic_add(int i, atomic_t *v)
    {
        asm volatile(LOCK_PREFIX "addl %1,%0"
                : "+m" (v->counter)
                : "ir" (i));
    }

    // ARM 架构实现
    static inline int atomic_add_return(int i, atomic_t *v)
    {
        unsigned long tmp;
        int result;

        smp_mb();

        __asm__ __volatile__("@ atomic_add_return\n"
    "1:	ldrex	%0, [%2]\n" // 独占内存指令
    "	add	%0, %0, %3\n"
    "	strex	%1, %0, [%2]\n"
    "	teq	%1, #0\n"
    "	bne	1b"
            : "=&r" (result), "=&r" (tmp)
            : "r" (&v->counter), "Ir" (i)
            : "cc");

        smp_mb();

        return result;
    }
    ```

- ``per-cpu``

    静态的 ``per-cpu`` 变量会在链接时被放置在ELF文件的特定段 ``.data..percpu`` 中，所有变量的布局是连续的，从 ``__per_cpu_start`` 开始到 ``__per_cpu_end`` 结束，会在运行时为每个 **cpu**  创建一个独立的内存副本。变量的访问通过计算偏移量实现，内核使用一个数组 ``__per_cpu_offset`` 存储每个 **cpu** 的偏移量。

    动态的 ``per_cpu`` 变量通过内核的动态内存分配机制实现，内核维护了一个专用的 ``per_cpu`` 内存池实现

    - 常用接口

        ```c++
        // 静态分配
        DECLARE_PER_CPU(type, name)
        get_cpu_var(var)
        put_cpu_val(var)
        // 动态分配
        alloc_percpu(type)  // 返回地址
        void free_percpu(void __percpu *ptr)
        get_cpu_ptr()
        put_cpu_ptr()
        per_cpu_ptr(ptr, cpu)
        ```

    - ``struct percpu_counter``

        ```c++
        // 一种内核编程技巧，减少对全局变量的访问
        struct percpu_counter {
            spinlock_t lock;
            s64 count;
        #ifdef CONFIG_HOTPLUG_CPU
            struct list_head list;	/* All percpu_counters are on a list */
        #endif
            s32 __percpu *counters;
        };

        /*
            内核为了尽可能少的加锁，使用了一些编程技巧，对计数器增加或者减少计数时，大多数情况下不用加锁，
            只修改每cpu变量s32 __percpu *counters，当计数超过一个范围时[-batch, batch],则进行加锁，
            将每cpu变量s32 __percpu *counters;中的计数累计到s64 count中。
        */

        // 常用的api
        percpu_counter_sum      //  返回精确值
        percpu_counter_read     //  返回粗略值
        percpu_counter_add      //  修改
        ```

- ``memory_barrier``

    确保内存操作执行顺序符合预期，防止由于处理器乱序执行或者编译器优化策略导致数据不一致的静态条件

    常见的比如 ``mb`` ``smp_mb`` ``smp_wmb`` ``smp_rmb``

    ```c++
    // 一些使用场景记录
    
    // case1: 多核数据共享
    // Core 1
    data = 42;
    smp_wmb();  // 写内存屏障
    flag = 1;

    // Core 2
    while (flag == 0);
    smp_rmb();  // 读内存屏障
    assert(data == 42);

    // case2: 锁实现
    lock();
    smp_mb();  // 获取锁后的屏障
    critical_section();
    smp_mb();  // 释放锁前的屏障
    unlock();
    ```

- ``spinlock_t``

    ``spinlock_t`` 简化的定义如下所示

    ```c++
    typedef struct raw_spinlock {
        arch_spinlock_t raw_lock;
    } raw_spinlock_t;

    typedef struct spinlock {
        struct raw_spinlock rlock; 
    } spinlock_t;
    ```

    `spin_lock` 源码解析

    ```c++
    static inline void spin_lock(spinlock_t *lock)
    {
        raw_spin_lock(&lock->rlock);
    }

    #define raw_spin_lock(lock)    _raw_spin_lock(lock)

    // 在UP环境中，由于同一时刻只有一个执行线程，自旋锁主要通过禁用抢占和中断来保证代码块的原子执行，而不是通过真正的自旋等待。
    // 在SMP环境中，自旋锁需要实现真正的自旋等待

    // UP中的实现：
    #define _raw_spin_lock(lock)            __LOCK(lock)
    #define __LOCK(lock) \
    do { preempt_disable(); __acquire(lock); (void)(lock); } while (0)

    // 通过preempt_disable 可以禁止抢占
    #define preempt_disable() \
    do { \
        inc_preempt_count(); \
        barrier(); \
    } while (0)

    // __acquire(lock) 用作静态代码检查，确保成对使用，默认情况下是一个空语句
    // (void)(lock) 空语句，防止编译器警告局部变量没有使用
    // do{ ... } whle(0) 保证宏语法的正确性，保证正确的代码块闭合

    // SMP的实现：
    #define _raw_spin_lock(lock) __raw_spin_lock(lock)
    static inline void __raw_spin_lock(raw_spinlock_t *lock)
    {
        preempt_disable();
        spin_acquire(&lock->dep_map, 0, 0, _RET_IP_);
        LOCK_CONTENDED(lock, do_raw_spin_trylock, do_raw_spin_lock);
    }

    static inline void do_raw_spin_lock(raw_spinlock_t *lock) __acquires(lock)
    {
        __acquire(lock);
        arch_spin_lock(&lock->raw_lock);
    }

    // ARM 架构下的arch_spin_lock实现
    typedef struct {
        volatile unsigned int lock;
    } arch_spinlock_t;

    static inline void arch_spin_lock(arch_spinlock_t *lock)
    {
        unsigned long tmp;

        __asm__ __volatile__(
    "1:	ldrex	%0, [%1]\n"             // 加载并且标记锁变量，和原子变量的实现类似，通过独占内存指令实现
    "	teq	%0, #0\n"                   // 测试锁是否为锁定状态，更新条件标志
    #ifdef CONFIG_CPU_32v6K       
    "	wfene\n"                        // wait for event指令，在自旋等待中减少能耗
    #endif
    "	strexeq	%0, %2, [%1]\n"         // 在 %0 为 0 的条件下执行，尝试将 %2 的值写入到lock中，并且把是否成功的信息保存在 0% 中
    "	teqeq	%0, #0\n"               // 检查是否成功获取到锁
    "	bne	1b"                         // 如果没有退回到1
        : "=&r" (tmp)
        : "r" (&lock->lock), "r" (1)
        : "cc");

        smp_mb();                       // 全屏障，确保在获取锁之后的任何指令不会被重排到获取锁之前
    }

    /*
    实际上针对不同的中断类型，spin_lock给出了不同的接口
    不会在任何中断例程中操作临界区: spin_lock
    软件/硬件中断可能会操作临界区: spin_lock_bh/spin_lock_irq
    ...
    */

    /*************************************ticket_based spin lock*************************************/

    /*
    实际上这种简单的实现，会带来极大的不公平性，由于存在缓存一致性的问题 ——> 释放锁之后，可能会导致其他cpu保存在 L1 Cache 中的数据失效，
    从而释放锁变量的 cpu 能够有更大的机会获取锁，这是一种不公平的竞争，下面展示了一种新的定义方式
    */
    // ticket_based spin lock 
    typedef struct {
        union {
            u32 slock;
            struct __raw_tickets {
                u16 next;  // 当前持有锁的票据号
                u16 owner;   // 下一个将要获取锁的票据号
            } tickets;
        };
    } arch_spinlock_t;

    static inline void arch_spin_lock(arch_spinlock_t *lock)
    {

        /*
        主要包括了三个动作：
            * 获取了自己的号码牌（next值）和允许哪一个号码牌进入临界区（owner）
            * 设定下一个进入临界区的号码牌（next++）
            * 判断自己的号码牌是否是允许进入的那个号码牌，如果是则进入，不是则等待 
        */

        unsigned long tmp;
        u32 newval;
        arch_spinlock_t lockval;

        prefetchw(&lock->slock);
        __asm__ __volatile__(
    "1:     ldrex   %0, [%3]\n"     /* 原子方式，读取锁的值赋值给lockval */
    "       add     %1, %0, %4\n"   /* 将next字段++之后的值存在newval中 */
    "       strex   %2, %1, [%3]\n" /* 原子方式，将新的值存在lock中，写入否成功结果存入在tmp中 */
    "       teq     %2, #0\n"       /* 判断是否写入成功，不成功跳到标号1重新执行 */
    "       bne     1b"
            : "=&r" (lockval), "=&r" (newval), "=&r" (tmp)
            : "r" (&lock->slock), "I" (1 << TICKET_SHIFT)
            : "cc");

        /* 查询是否可以拿锁，若next != owner说明已有人持锁，自旋 */
        while (lockval.tickets.next != lockval.tickets.owner) {
            wfe();  // 降低等待消耗
            lockval.tickets.owner = READ_ONCE(lock->tickets.owner);
        }
    
        smp_mb();   
    }
    
    /* 释放锁比较简单，将owner++即可 */
    static inline void arch_spin_unlock(arch_spinlock_t *lock)
    {
        smp_mb();
        lock->tickets.owner++;
        dsb_sev();
    }

    /*************************************MSC spin lock*************************************/

    // ticket_based spin lock 解决了公平性的问题，但是在性能上还存在一些问题。由于多个 CPU 线程均在同一个共享变量 lock.slock 上自旋，
    // 而申请和释放必须对 lock.slock 进行修改，这将导致所有参与排队的处理器的缓存变得无效，在锁竞争激烈的情况下，频繁的缓存同步操作将会导致
    // 饭中的系统总线和内存流量，大大降低系统性能

    // MSC spin lock 设计思想：每个锁的等待者在本 CPU 上自旋，访问本地变量，而不是全局的 spinlock 变量

    /*
    * struct mcs_node结构体用于描述本地节点，mcs_node中包含2个变量，next指针用于指向下一个等待者，而另外一个变量，则用于自旋锁的自旋。
    * 很明显，mcs_node结构可以让所有等待者变成一个单向链表。
    */
    struct mcs_node {
        struct mcs_node *next;   /* 指向下一个等待者，通过这种方式能够减少缓存行的震荡 */
        int is_locked;           /* 本地自旋变量 */
    }
    
    /*
    * 全局spinlock中含有一个mcs_node指针，指向最后一个锁的申请者。而当锁处于空闲时，该指针为NULL。
    */
    struct spinlock_t {
        mcs_node *queue;    
    }
    
    //加锁函数
    mcs_spin_lock(spinlock_t *lock, mcs_node *my_node)
    {
        my_node->next = NULL;               
        mcs_node *predecessor = fetch_and_store(lock->queue, my_node);  
        if (predecessor != NULL) {        
            my_node->is_locked = true;     
            predecessor.next = my_node;   
            while (my_node->is_locked)     
                cpu_relax();
        }
    }


    //放锁函数
    mcs_spin_unlock(spinlock_t *lock, mcs_node *my_node)
    {
        if (my_node->next == NULL) {        
            if (compare_and_swap(lock->queue, my_node, NULL)) { 
                return;
            }
            else {
                while (my_node->next == NULL) 
                    cpu_relax();
            }
        }
        my_node->next->is_locked = false;    
    }

    /*************************************Queue spin lock*************************************/ 

    typedef struct qspinlock {
	union {
		atomic_t val;
 
        struct {
            u8	locked;
            u8	pending;
        };
        struct {
            u16	locked_pending;
            u16	tail;
        };
    }
    } arch_spinlock_t;
    ```

- ``rwlock_t``

    简化的定义如下所示：

    ```c++
    typedef struct {
        arch_rwlock_t raw_lock;
    } rwlock_t;
    
    // arm 架构下的定义如下
    typedef struct {
	    volatile unsigned int lock;
    } arch_rwlock_t;
    ```

    具体实现分析

    ```c++
    // UP环境下的rwlock实现和UP环境的spinlock实现没有区别

    // arch/arm/include/asm/spinlock.h
    // rwlock 的 write_lock 和 write_unlock 的实现和 spin_lock基本一致，最终会进入到arch_write_lock函数
    static inline void arch_write_lock(arch_rwlock_t *rw)
    {
        unsigned long tmp;

        __asm__ __volatile__(
            ...
        : "r" (&rw->lock), "r" (0x80000000)
        // 写入0x0x80000000表示被一个写者拥有
        : "cc");

        smp_mb();
    }

    static inline void arch_write_unlock(arch_rwlock_t *rw)
    {
        smp_mb();

        __asm__ __volatile__(
        "str	%1, [%0]\n"
        :
        : "r" (&rw->lock), "r" (0)
        : "cc");

        dsb_sev(); // barrier + send event
    }

    static inline void arch_read_lock(arch_rwlock_t *rw)
    {
        unsigned long tmp, tmp2;

        __asm__ __volatile__(
    "1:	ldrex	%0, [%2]\n"             // 把rw->lock的值加载到寄存器 %0 中                  
    "	adds	%0, %0, #1\n"           // 将 %0 寄存器中的值加 1 同时更新条件标志位
    "	strexpl	%1, %0, [%2]\n"         // strexpl指令会在满足条件（之前的adds指令没有被设置负标志）执行
    #ifdef CONFIG_CPU_32v6K
    "	wfemi\n"                        // 在支持该指令的架构中，减少等待循环的能耗，使处理器进入一个事件等待模式
    #endif
    "	rsbpls	%0, %1, #0\n"           // 如果上一条strex指令不成功， %1 不为 0，这条指令把 %0 设置为 -%1，如果成功，不执行该指令
    "	bmi	1b"                         // 如果最后的结果为负，重回1
        : "=&r" (tmp), "=&r" (tmp2)
        : "r" (&rw->lock)
        : "cc");

        smp_mb();
    }

    static inline void arch_read_unlock(arch_rwlock_t *rw)
    {
        unsigned long tmp, tmp2;

        smp_mb();

        __asm__ __volatile__(
    "1:	ldrex	%0, [%2]\n"
    "	sub	%0, %0, #1\n"
    "	strex	%1, %0, [%2]\n"
    "	teq	%1, #0\n"
    "	bne	1b"
        : "=&r" (tmp), "=&r" (tmp2)
        : "r" (&rw->lock)
        : "cc");

        if (tmp == 0)
            dsb_sev();
    }
    ```

- ``seqlock_t``

    >读操作不用加锁，写操作通过自旋锁保护，非常适合读多写少的场景

    通过奇偶性判断是否存在写进程

    ```c++
    typedef struct {
        unsigned sequence;
        spinlock_t lock;
    } seqlock_t;

    static inline void write_seqlock(seqlock_t *sl)
    {
        spin_lock(&sl->lock);
        ++sl->sequence;
        smp_wmb();  // 确保序列号增加操作完成
    }

    static inline void write_sequnlock(seqlock_t *sl)
    {
        smp_wmb();  // 确保写操作完成
        sl->sequence++;
        spin_unlock(&sl->lock);
    }

    static __always_inline unsigned read_seqbegin(const seqlock_t *sl)
    {
        unsigned ret;

    repeat:
        ret = sl->sequence;
        smp_rmb();
        if (unlikely(ret & 1)) {
            cpu_relax();
            goto repeat;
        }

        return ret;
    }

    static __always_inline int read_seqretry(const seqlock_t *sl, unsigned start)
    {
        smp_rmb();

        return (sl->sequence != start);
    }

    ```

- ``rcu``

    随着计算机硬件技术的发展，CPU运算速度越来越快，相比之下， 存储器件的速度发展较为滞后，在这种背景下，获取基于 **Counter** 机制的锁的开销比较大，无法满足性能的需求

    适用场景

    - RCU 只能保护动态分配的数据结构，并且必须是通过指针访问该数据结构
    - RCU 保护的临界区内不能 **sleep**
    - 读写不对称，对写的性能没有特别要求，但是对读的性能要求极高
    - 读端对新旧数据不敏感

    ```c++
    struct rcu_head { // read copy update  -> 随意读，但更新数据的时候，需要先复制一份副本，在副本上修改，在一次性的替换旧数据
        struct rcu_head *next;
        void (*func)(struct rcu_head *head);
    };

    /*
        核心机制在于宽期限和订阅-发布机制
    */

    // 核心API    
    rcu_read_lock
    rcu_read_unlock
    synchronize_rcu     // 核心所在，等带读者退出
    rcu_assign_pointer  // 写者调用该函数为被RCU保护的指针分配一个新的值
    rcu_dereference     // 读者调用它来获得一个被RCU保护的指针
    ```

- ``mutex``

    ```c++
    struct mutex {
        atomic_long_t		owner;              //原子计数，用于指向锁持有者的task struct结构
        spinlock_t		wait_lock;              //自旋锁，用于wait_list链表的保护操作
    #ifdef CONFIG_MUTEX_SPIN_ON_OWNER
        struct optimistic_spin_queue osq;       //osq锁
    #endif
        struct list_head	wait_list;          //链表，用于管理所有在该互斥锁上睡眠的进程
        ...                                     // for Debug
    };

    /*
    Fast-path:
        这是最快的路径，当没有竞争时使用。它尝试通过一个原子操作（通常是 cmpxchg）直接将 owner 设置为当前任务。如果成功，进程就获取了锁而不需要进一步的操作。
    Mid-path:
        如果 fast-path 失败，互斥锁代码将尝试 mid-path，其中包括使用 OSQ 锁。这时，系统会检查锁的持有者是否正在运行在同一 CPU 上。如果是，执行自旋等待，因为锁很可能很快就会被释放。
    Slow-path:
        如果锁不能在 mid-path 被获取，控制流进入 slow-path。在这个路径上，进程将被加入到 wait_list，并进入睡眠状态，直到锁变为可用。这需要对 wait_list 进行加锁操作，通常使用 wait_lock。

    */
    ```

- ``semaphore``

- ``rw_semaphore``

- ``Lockdep``

### 文件管理

- ``files_struct`` ``fdtable`` ``file``

    每个进程会维护一个 ``files_struct`` 来记录该进程打开文件的信息

    ```c++
    struct fdtable {
        unsigned int max_fds;
        struct file ** fd;      /* current fd array */
        fd_set *close_on_exec;
        fd_set *open_fds;
        struct rcu_head rcu;
        struct fdtable *next;
    };
    ```

    ```c++
    struct files_struct {
    /*
    * read mostly part
    */
        atomic_t count;
        struct fdtable *fdt;
        struct fdtable fdtab;        
    /*
    * written part on a separate cache line in SMP
    */
        spinlock_t file_lock ____cacheline_aligned_in_smp;
        int next_fd;
        struct embedded_fd_set close_on_exec_init;
        struct embedded_fd_set open_fds_init;
        struct file * fd_array[NR_OPEN_DEFAULT];
    };
    ```

    ![fdtable files_struct示意图](./imags/fdtable%20files_struct.png)

    系统中的每个打开的文件在内核空间都有一个关联的 ``file`` , 在文件的所有实例都关闭后，内核会释放这个结构

    一个磁盘上的文件可以对应多个 ``file``

    ```c++
    struct file {
        union {
            struct list_head	fu_list;
            struct rcu_head 	fu_rcuhead;
        } f_u;
        struct path		f_path;
    #define f_dentry	f_path.dentry
    #define f_vfsmnt	f_path.mnt
        const struct file_operations	*f_op;
        spinlock_t		f_lock;         /* f_ep_links, f_flags, no IRQ */
        atomic_long_t		f_count;    // 文件计数
        unsigned int 		f_flags;    // 文件标志
        fmode_t			f_mode;         // 访问模式
        loff_t			f_pos;          // 文件偏移量
        struct fown_struct	f_owner;    // 与异步IO通知相关
        const struct cred	*f_cred;    // 打开文件使用的安全凭证
        struct file_ra_state	f_ra;   // 用于管理文件的预读取状态
        ...
    };

    ```

- ``dentry ( directory entry )``

    ``inode`` 仅仅保存了文件对象的属性信息，包括权限、属组、数据块的位置、时间戳等，但是没有包含文件名，通过 ``dentry`` 能够在内存中维护文件系统的目录树，是一个纯粹的内存结构，由文件系统提供文件访问的过程中直接在内存建立

    ``inode``可以理解为对应于物理磁盘上的具体对象， ``dentry`` 是一个内存实体，每个 ``dentry`` 都有唯一对应的 ``inode``

    ```c++
    struct dentry {
        atomic_t d_count;
        unsigned int d_flags;		/* protected by d_lock */
        spinlock_t d_lock;		    /* per dentry lock */
        int d_mounted;
        struct inode *d_inode;		/* Where the name belongs to - NULL is negative */
        /*
        * The next three fields are touched by __d_lookup.  Place them here
        * so they all fit in a cache line.
        */
        struct hlist_node d_hash;	/* lookup hash list */
        struct dentry *d_parent;	/* parent directory */
        struct qstr d_name;

        struct list_head d_lru;		/* LRU list */
        /*
        * d_child and d_rcu can share memory
        */
        union {
            struct list_head d_child;	/* child of parent list */
            struct rcu_head d_rcu;
        } d_u;
        struct list_head d_subdirs;	/* our children */
        struct list_head d_alias;	/* inode alias list */
        unsigned long d_time;		/* used by d_revalidate */
        const struct dentry_operations *d_op;
        struct super_block *d_sb;	/* The root of the dentry tree */
        void *d_fsdata;			    /* fs-specific data */

        unsigned char d_iname[DNAME_INLINE_LEN_MIN];	/* small names */
    };
    ```

- ``vfs_mount``

    ```c++
    struct vfsmount {
        struct list_head mnt_hash;
        struct vfsmount *mnt_parent;	/* fs we are mounted on */
        struct dentry *mnt_mountpoint;	/* dentry of mountpoint */
        struct dentry *mnt_root;	/* root of the mounted tree */
        struct super_block *mnt_sb;	/* pointer to superblock */
        struct list_head mnt_mounts;	/* list of children, anchored here */
        struct list_head mnt_child;	/* and going through their mnt_child */
        int mnt_flags;
        /* 4 bytes hole on 64bits arches */
        const char *mnt_devname;	/* Name of device e.g. /dev/dsk/hda1 */
        struct list_head mnt_list;
        struct list_head mnt_expire;	/* link in fs-specific expiry list */
        struct list_head mnt_share;	/* circular list of shared mounts */
        struct list_head mnt_slave_list;/* list of slave mounts */
        struct list_head mnt_slave;	/* slave list entry */
        struct vfsmount *mnt_master;	/* slave is on master->mnt_slave_list */
        struct mnt_namespace *mnt_ns;	/* containing namespace */
        int mnt_id;			/* mount identifier */
        int mnt_group_id;		/* peer group identifier */
        /*
        * We put mnt_count & mnt_expiry_mark at the end of struct vfsmount
        * to let these frequently modified fields in a separate cache line
        * (so that reads of mnt_flags wont ping-pong on SMP machines)
        */
        atomic_t mnt_count;
        int mnt_expiry_mark;		/* true if marked for expiry */
        int mnt_pinned;
        int mnt_ghosts;
    #ifdef CONFIG_SMP
        int __percpu *mnt_writers;
    #else
        int mnt_writers;
    #endif
    };
    ```

- ``path``

    ```c++
    struct path {
        struct vfsmount *mnt;
        struct dentry *dentry;
    };
    ```

- ``fs_struct``

    ```c++
    struct fs_struct {
        int users;
        rwlock_t lock;
        int umask;
        int in_exec;
        struct path root, pwd;
    };
    ```

- ``inode``

- ``super_block``

- ``vs_area_struct``

### 进程管理

- ``task_struct``

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

## 进程控制

### `fork` `vfork` `clone`

- 调用方式

    ```c++
    pid_t fork(void);
    pid_t vfork(void);
    pid_t clone(int (*fn)(void *), void *child_stack, int flags, void *arg, ...);

    // ... -> do_fork -> copy_process
    ```

- 源码剖析

    ```c++
    // copy_process -> copy_files -> dup_fd

    // -------------------copy_files 代码截取-------------------
    ...

    if (clone_flags & CLONE_FILES) {
        atomic_inc(&oldf->count);
        goto out;
    }   // 可以看到这里如果标记了CLONE_FILES，子进程不会创建一个新的files_struct实例，能够实现高效的进程分叉

    newf = dup_fd(oldf, &error);

    // -------------------dup_fd 代码截取-------------------

	for (i = open_files; i != 0; i--) {
		struct file *f = *old_fds++;
		if (f) {
			get_file(f); // 这里只是增加了对应file结构体的引用计数，而没有产生真的复制，所以fork出来的子进程实际上会共享父进程的打开文件副本
		} else {
			FD_CLR(open_files - i, new_fdt->open_fds);
		}
		rcu_assign_pointer(*new_fds++, f);
	}
    ...

    ```

### `wait`

```c++
pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);

// waitpid可以指定要回收的进程，并且通过options参数实现了更精细的控制

/*
WNOHANG         ： 使waitpid成为非阻塞调用，如果指定的子进程没有结束，waitpid会立即返回0
WUNTRACED       ： 除了返回已终止的子进程信息外，还返回因信号而停止的子进程的信息
WCONTINUED      ： 返回哪些已经由停止状态继续执行但还未终止的子进程状态
WNOWAIT         ：（在某些系统中支持）是的调用清除子进程的状态，子进程状态仍然可以用于后续的waitpid调用
*/

int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);

// idtype : P_PID, P_PGID, P_ALL
// options     ： WCONTINUED WEXITED WNOHANG WNOWAIT WSTOPPED  能够实现更精细的控制

pid_t wait3(int *statloc, int options, struct rusage *rusage);
pid_t wait4(pid_t pid, int *staloc, int options, struct rusage *rusage);
// 通过rusage能够返回更多的信息
```

### ``exec``

![exec族函数关系](./imags/exec.png)

## 进程关系

### 进程组

```c++
// 作为一个统一接受信号的集合
// 每个进程组有一个组长进程，组长进程的进程组id和进程id相等，没有自动组长替代机制，如果组长进程终止或者离开进程组，不会产生新的组长
// 只要某个进程组中有一个进程存在，则该进程组就存在

pid_t getpgid(pid_t pid);
int setpgid(pid_t pid, pid_t pgid);
// 如果两个参数相等，那么pid指定的进程变成进程组组长
// 如果pid为0，那么为自己指定进程组，如果pgid为0，那么pid指定的进程id用作进程组id
// 一个进程只能为它或它的子进程设置进程组ID，在他的子进程调用了exec之后就不能再更改子进程的进程组ID
```

### 会话

```c++
pid_t setsid(void);
/*
如果此函数的进程不是一个进程组的组长，则此函数创建一个新会话，具体发生下面三件事
    * 该进程变成新会话的首进程
    * 该进程成为一个新进程组的组长进程
    * 该进程没有控制终端，如果在调用setsid之前该进程有一个控制终端，那么这种联系也被切断
*/
pid_t getsid(pid_t pid);
// 如果pid并不属于调用者所在的会话，那么调用进程就不能得到该会话首进程进程组ID
```

### 控制终端

一个会话可以有一个控制终端

建立与控制终端连接的会话首进程被称为控制进程

一个会话中进程组可以被分为前台进程组和多个后台进程组

无论何时键入终端的中断键或是退出键，都会将退出信号发送至前台进程组的所有进程

## 信号

### 信号说明

```c++
#define SIGHUP          1   // 终端控制进程结束时发送给相应进程
#define SIGINT          2   // 中断信号，通常由用户在终端按下Ctrl+C触发
#define SIGQUIT         3   // 退出信号，由Ctrl+\触发，通常导致进程终止并产生核心转储
#define SIGILL          4   // 非法指令信号，执行非法指令时发送
#define SIGTRAP         5   // 由断点指令或其他陷阱指令产生
#define SIGABRT         6   // 由abort()函数产生，退出信号
#define SIGIOT          6   // 同SIGABRT，退出信号
#define SIGBUS          7   // 总线错误，非法地址访问
#define SIGFPE          8   // 浮点异常，如除零操作
#define SIGKILL         9   // 杀死信号，用于立即结束程序执行   不能被忽略
#define SIGUSR1        10   // 用户自定义信号1
#define SIGSEGV        11   // 段错误信号，如访问未分配的内存
#define SIGUSR2        12   // 用户自定义信号2
#define SIGPIPE        13   // 向一个没有读端的管道写数据
#define SIGALRM        14   // 由alarm()函数设置的定时器超时时发送
#define SIGTERM        15   // 终止信号，用于结束程序
#define SIGSTKFLT      16   // 栈故障
#define SIGCHLD        17   // 子进程结束或停止时，发送给其父进程，默认行为是忽略
#define SIGCONT        18   // 使停止的进程继续执行
#define SIGSTOP        19   // 停止进程的执行，非终止         不能被忽略
#define SIGTSTP        20   // 停止信号，如Ctrl+Z
#define SIGTTIN        21   // 后台进程读终端
#define SIGTTOU        22   // 后台进程写终端
#define SIGURG         23   // 紧急情况信号，如网络上的紧急数据到达
#define SIGXCPU        24   // 超过CPU时间限制
#define SIGXFSZ        25   // 超过文件大小限制
#define SIGVTALRM      26   // 虚拟定时器到期
#define SIGPROF        27   // 实际时间定时器到期
#define SIGWINCH       28   // 窗口大小变化
#define SIGIO          29   // I/O现在可能进行
#define SIGPOLL        SIGIO  // 同SIGIO，轮询可能发生
#define SIGPWR         30   // 电源故障
#define SIGSYS         31   // 非法的系统调用
#define SIGUNUSED      31   // 未使用的信号（不再使用）
#define SIGRTMIN       32   // 实时信号范围的最小值
#define SIGRTMAX       _NSIG  // 实时信号范围的最大值，具体数值取决于系统

```

### ``signal`` 函数

```c++
void (*signal(int signo, void (*func)(int)))(int);
// 成功返回以前的信号处理配置，出错返回SIG_ERR
// SIG_IGN  -> #define SIG_IGN (void (*)()) 1表示忽略信号
// SIG_DFL  -> #define SIG_DFL (void (*)()) 0表示将信号的处理方式回复默认行为


// 启动一个程序的时候所有信号的状态都是系统默认或忽略，创建进程的时候，子进程会继承父进程的信号处理方式
// 信号处理函数不能调用不可重入函数，不然可能会导致不可预计的后果，调用 malloc 可能会导致死锁（重复获取不可重入锁）
```

### ``kill`` 和 ``raise``

```c++
int kill(pid_t pid, int signo);
/*
    pid > 0 将信号发送给进程ID为pid的进程
    pid = 0 将信号发送给同组进程，要有权限
    pid < 0 按组发送信号
    pid = -1 将信号发送给所有有权限发送的进程  
*/
int raise(int signo);
```

### ``alarm`` 和 ``pause``

```c++
unsigned int alarm(unsigned int seconds);
// 在将来某个时刻决定定时器会超时，产生SIGALRM信号，如果忽略或不捕获此信号，默认是终止调用alarm的进程
// 一个进程只能有一个闹钟时间，如果在调用alarm的时候，之前为该进程注册的闹钟函数还没有超时，则返回上次的剩余时间，并且更新新的alarm值

int pause(void);
// 只有执行了一个信号处理函数并且从其返回时候，pause才返回，在这种情况，pause返回-1，errno设置为EINTR
```

### 信号集

```c++
int sgiemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signo);
int sigdelset(sigset_t *set, int signo);
int sigismember(const sigset_t *set, int signo);
```

### ``sigprocmask``

```c++
int sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);
// how 决定了如何修改当前的信号屏蔽字
// SIG_BLOCK
// SIG_UNBLOCK
// SIG_SETMASK
```

### ``sigpending``

```c++
int sigpending(sigset_t *set);
// 用于检查当前进程的挂起信号，可以了解哪些信号已经发送给进程但是由于某种原因尚未被处理
```

### ``sigaction``

```c++
int sigaction(int signo, const struct sigaction *restrict act, struct sigaction *restrict oact);

struct sigaction {
    void     (*sa_handler)(int);
    void     (*sa_sigaction)(int, siginfo_t *, void *);     // 另一个信号处理函数，提供了更多信息传递
    sigset_t   sa_mask;                                     // 指定在当前信号处理函数执行期间需要阻塞的附加信号
    int        sa_flags;            // 信号处理的各种选项和行为，常见的有SA_RESTART SA_NOCLDSTOP SA_SIGINFO
    void     (*sa_restorer)(void);  // 不再使用的字段
};

typedef struct siginfo {
    int      si_signo;     /* Signal number */
    int      si_errno;     /* An errno value */
    int      si_code;      /* Signal code */
    int      si_trapno;    /* Trap number that caused
                              hardware-generated signal
                              (unused on most architectures) */
    pid_t    si_pid;       /* Sending process ID */
    uid_t    si_uid;       /* Real user ID of sending process */
    int      si_status;    /* Exit value or signal */
    clock_t  si_utime;     /* User time consumed */
    clock_t  si_stime;     /* System time consumed */
    sigval_t si_value;     /* Signal value */
    int      si_int;       /* POSIX.1b signal */
    void    *si_ptr;       /* POSIX.1b signal */
    int      si_overrun;   /* Timer overrun count; POSIX.1b timers */
    int      si_timerid;   /* Timer ID; POSIX.1b timers */
    void    *si_addr;      /* Memory location which caused fault */
    long     si_band;      /* Band event */
    int      si_fd;        /* File descriptor */
    short    si_addr_lsb;  /* Least significant bit of address
                              (since Linux 2.6.32) */
} siginfo_t;

```

### ``abort``

此函数将 ``SIGABRT`` 信号发送给调用进程，ISO C 要求若捕捉次信号而且相应信号处理程序返回， ``abort`` 仍不会回到其调用者

## 线程

### 线程**Id**

```c++
int pthread_equal(pthread_t tid1, pthread_t tid2);
pthread_t pthread_self(void);
```

### 线程创建

```c++
int pthread_create(pthread_t *restrict tidp, const pthread_attr_t *restrict attr, void *(*start_rtn)(void *), void *restrict arg);
/*
tidp:       指向 pthread_t 类型的指针，该类型用于唯一标识新创建的线程。成功创建线程后，该标识符被写入此位置。
attr:       指向 pthread_attr_t 结构的指针，该结构定义了线程的属性（如堆栈大小、调度策略等）。如果传递 NULL，则使用默认线程属性。
start_rtn   指向函数的指针，该函数作为线程启动时执行的新例程。函数必须接受一个 void 类型的指针，并返回一个 void 类型的指针。
arg:        传递给 start_rtn 函数的单一参数。这可以用来提供多种数据给线程。
*/

/*
pthread_attr_t:     一个用于线程属性的数据结构。它通常在创建线程之前设置，并可以指定线程的多种属性，如堆栈大小、调度策略、继承调度、作用域等。

常用函数:
- pthread_attr_init:          初始化 pthread_attr_t 结构体到默认值。
- pthread_attr_destroy:       释放 pthread_attr_t 结构体占用的资源。
- pthread_attr_set/getstacksize:  设置线程的堆栈大小。
- pthread_attr_set/getscope:      设置线程的作用域（系统或进程间）。
- pthread_attr_set/getschedpolicy:设置线程的调度策略（如 SCHED_FIFO, SCHED_RR）。
- pthread_attr_set/getdetachstate:线程的分离状态（分离或非分离）。
*/
```

### 线程终止

```c++
void pthread_exit(void *retval);

/*
retval:     指针，指向返回值，这个返回值可以被同一进程中的其他线程通过 pthread_join() 函数接收。如果线程不需要提供返回值，则可以传递 NULL。
*/

int pthread_join(pthread_t thread, void **retval);
/*
thread:     要等待的线程的标识符，该标识符由 pthread_create 函数返回。
retval:     指向指针的指针，用于存储线程通过 return 语句或 pthread_exit() 返回的退出状态。如果不关心线程的返回值，则可以设置为 NULL。
*/
// 调用线程将一直被阻塞，直到指定的线程调用pthread_exit、从启动例程中返回或者被取消，前两种方式会将返回值传递给retval，第三种会将指向的内存位置设置为PTHREAD_CANCELED
// 在pthread_join被调用之后，即使没有显示的调用pthred_detach，被join的线程资源也会自动释放，尝试对一个detach线程join调用会失败 

int pthread_cancel(pthread_t thread);

/*
thread:    要取消的线程的标识符。这是一个 pthread_t 类型的值，通常由 pthread_create() 函数返回。

描述: 请求取消同一进程中的另一个线程。被取消的线程并不会立即终止，取消请求会在线程到达某个取消点时生效，除非线程设置了异步取消能力。线程取消是协作性的，线程需要定期检查是否已被取消，并相应地清理资源。
返回值:   成功时返回 0；失败时返回错误码，如 ESRCH 表示没有找到相应线程。
*/

void pthread_cleanup_push(void (*routine)(void *), void *arg);

/*
routine:    指向要执行的清理函数的指针。此函数由线程调用，当线程退出时，无论是通过 pthread_exit 调用，还是响应取消请求，都会执行。
arg:        传递给清理函数的参数。这可以是指向任何类型的数据的指针，通常用于传递必须释放或清理的资源的信息。

描述:       pthread_cleanup_push 用于注册一个在线程终止时自动执行的清理处理程序。这个处理程序将被放置在一个堆栈上，线程终止时将按照后进先出的顺序执行。
注意        从启动例程中返回而终止的情况下，清理程序不会被调用，exit或者cancel的情况才会被调用
*/

void pthread_cleanup_pop(int execute);

/*
execute:    一个整数值，用于指示是否立即执行清理处理程序。
            - 如果为 0，则仅从清理堆栈中移除处理程序，不执行。
            - 如果非 0，则从堆栈中移除处理程序，并执行它。

描述:       pthread_cleanup_pop 用于移除最近注册的清理处理程序。根据传递给 execute 参数的值，它可以选择性地执行清理处理程序。通常与 pthread_cleanup_push 配对使用。
注意        在一些实现中，pthread_cleanup_push 和 pthread_cleanup_pop 需要在同一词法作用域内使用，因为它们可能被实现为宏，使用了 { 和 } 对于作用域的处理。这就要求它们必须在同一个函数中配对使用。
*/

int pthread_detach(pthread_t thread);

/*
thread:     要分离的线程的标识符，该标识符由 pthread_create 函数返回。

描述:       pthread_detach 函数用于将指定的线程置于分离状态。分离状态的线程在终止时会自动释放所有资源，包括线程描述符和堆栈。这意味着一旦线程终止，它的资源和状态不能通过 pthread_join 来回收或检查。

返回值:    成功时返回 0；如果失败，则返回一个错误码，例如：
            - EINVAL：指定的线程标识符不是一个正在运行的线程。
            - ESRCH：没有找到与给定线程标识符相对应的线程
*/
```

### 线程同步

#### 互斥量

```c++
// 静态分配
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// 动态分配
pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
pthread_mutex_init(mutex, NULL); // 使用默认属性初始化互斥锁


int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);

/*
mutex:      指向互斥锁的指针。
attr:       指向互斥锁属性的指针。如果设置为 NULL，互斥锁使用默认属性。
注意事项:   互斥锁必须在使用前初始化。如果 attr 是 NULL，互斥锁将被初始化为默认属性。
*/
int pthread_mutex_destroy(pthread_mutex_t *mutex);

/*
mutex:      指向互斥锁的指针。
注意事项:   销毁互斥锁后，不应再使用它，除非再次初始化。尝试销毁一个正在被锁定的互斥锁会导致不可预测的行为。
*/

int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);

/*
mutex:      指向互斥锁的指针。pthread_mutex_lock 用于获取互斥锁，而 pthread_mutex_unlock 用于释放互斥锁。
trylock注意事项:   尝试锁定互斥锁而不阻塞。如果互斥锁已经被其他线程锁定，函数将立即返回一个非零值（通常是EBUSY）。如果成功获取锁，返回0。
*/
```

#### `pthread_mutex_timedlock`

```c++
int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *timeout);

/*
mutex:      指向互斥锁的指针。
timeout:    指向 `timespec` 结构的指针，该结构定义了锁定操作应超时的绝对时间。

注意事项:   函数尝试锁定互斥锁。如果互斥锁已经被另一个线程锁定，调用线程将阻塞直到互斥锁变为可用或直到超过指定的 timeout 时间。如果在指定时间内互斥锁未能被锁定，则返回 ETIMEDOUT 错误码。这个函数对于实现具有超时的同步控制是非常有用的。
*/

```

#### 读写锁

```c++
int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr);

/*
attr:       指向读写锁属性的指针，可用于定义锁的行为。如果传入 NULL，使用默认属性。

注意事项:   读写锁在使用前必须初始化。attr 参数允许定义锁的行为，例如偏向读者或写者。
*/

int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);

/*
注意事项:   销毁后的读写锁不应再被使用，除非重新初始化。尝试销毁一个正在被使用的读写锁可能导致未定义行为。
*/

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);

/*
注意事项:   请求以读模式锁定读写锁。如果锁已被写者占用或等待，则调用线程将阻塞，直到可以获得读锁。
*/

int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);

/*
注意事项:   请求以写模式锁定读写锁。如果锁已被其他读者或写者占用，则调用线程将阻塞，直到可以获得写锁。
*/

int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);

/*
注意事项:   解锁读写锁。只有锁定锁的线程才能解锁它。尝试解锁未被锁定的读写锁可能导致未定义行为。
*/

int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);

/*
注意事项:   尝试以读模式锁定读写锁，但不阻塞。如果锁立即可用，则返回0，否则返回EBUSY。
*/

int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);

/*
注意事项:   尝试以写模式锁定读写锁，但不阻塞。如果锁立即可用，则返回0，否则返回EBUSY。
*/
```

#### 带有超时的读写锁

```c++
int pthread_rwlock_timedrdlock(pthread_rwlock_t *rwlock, const struct timespec *timeout);

/*
rwlock:     指向读写锁变量的指针。
timeout:    指定锁操作应该超时的绝对时间。

注意事项:   尝试以读模式锁定读写锁，但带有超时。如果在指定的时间内未能获取锁，则返回ETIMEDOUT。
*/

int pthread_rwlock_timedwrlock(pthread_rwlock_t *rwlock, const struct timespec *timeout);

/*
rwlock:     指向读写锁变量的指针。
timeout:    指定锁操作应该超时的绝对时间。

注意事项:   尝试以写模式锁定读写锁，但带有超时。如果在指定的时间内未能获取锁，则返回ETIMEDOUT。
*/

```

#### 条件变量

```c++
int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);

/*
cond:       指向条件变量的指针。
attr:       指向条件变量属性的指针，可用于设置条件变量的属性。如果为 NULL，使用默认属性。

注意事项:   条件变量在使用前必须初始化。如果 attr 是 NULL，则条件变量被初始化为默认属性。
*/
int pthread_cond_destroy(pthread_cond_t *cond);

/*
cond:       指向条件变量的指针。

注意事项:   销毁后的条件变量不应再被使用，除非重新初始化。尝试销毁正在等待的条件变量可能导致未定义行为。
*/
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);

/*
cond:       指向条件变量的指针。
mutex:      指向已锁定的互斥锁的指针。

注意事项:   调用线程在条件变量上等待，同时释放互斥锁。当被唤醒时，互斥锁将再次被锁定。这需要与 pthread_cond_signal 或 pthread_cond_broadcast 结合使用。
*/
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *timeout);

/*
cond:       指向条件变量的指针。
mutex:      指向已锁定的互斥锁的指针。
timeout:    指定等待应超时的绝对时间。

注意事项:   类似于 pthread_cond_wait，但有超时限制。如果在指定时间内条件未被触发，则返回 ETIMEDOUT。
*/
int pthread_cond_signal(pthread_cond_t *cond);

/*
cond:       指向条件变量的指针。

注意事项:   唤醒等待该条件变量的至少一个线程。如果没有线程在等待，该调用无效果。
*/
int pthread_cond_broadcast(pthread_cond_t *cond);

/*
cond:       指向条件变量的指针。

注意事项:   唤醒等待该条件变量的所有线程。如果没有线程在等待，该调用无效果。
*/

```

#### 自旋锁

```c++
int pthread_spin_init(pthread_spinlock_t *lock, int pshared);

/*
lock:       指向自旋锁变量的指针。
pshared:    指示自旋锁是否应该在多个进程间共享。如果值为PTHREAD_PROCESS_PRIVATE（默认），锁只在同一进程的线程间共享。如果值为PTHREAD_PROCESS_SHARED，则可在多个进程间共享。

注意事项:   自旋锁在使用前必须初始化。这个函数初始化自旋锁，设置它的共享模式。
*/

int pthread_spin_destroy(pthread_spinlock_t *lock);

int pthread_spin_lock(pthread_spinlock_t *lock);
int pthread_spin_trylock(pthread_spinlock_t *lock);
int pthread_spin_unlock(pthread_spinlock_t *lock);
```

#### 屏障

```c++
int pthread_barrier_init(pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, unsigned int count);

/*
barrier:    指向屏障变量的指针。
attr:       指向屏障属性对象的指针，如果为 NULL，则使用默认属性。
count:      指定必须到达屏障的线程数量，即屏障的参与者数量。

注意事项:   屏障在使用前必须初始化。count 参数指定了必须调用 pthread_barrier_wait 才能触发屏障的线程数量。
*/

int pthread_barrier_destroy(pthread_barrier_t *barrier);

/*
barrier:    指向屏障变量的指针。

注意事项:   销毁屏障后，不应再使用它，除非重新初始化。尝试销毁一个正在使用中的屏障可能导致未定义行为。
*/

int pthread_barrier_wait(pthread_barrier_t *barrier);

/*
barrier:    指向屏障变量的指针。

注意事项:   当调用此函数的线程数量达到初始化屏障时指定的 count 时，屏障开放，所有等待的线程将继续执行。
            函数返回 PTHREAD_BARRIER_SERIAL_THREAD 给一个线程，其他线程收到 0。返回 PTHREAD_BARRIER_SERIAL_THREAD
            的线程可以执行一些清理任务。
*/

```

## 线程控制

### 线程属性

```c++
int pthread_attr_init(pthread_attr_t *attr);

/*
attr:       指向线程属性对象的指针。
注意事项:   在设置任何线程属性之前，必须先初始化线程属性对象。此函数将线程属性设置为默认值。
*/

int pthread_attr_destroy(pthread_attr_t *attr);

/*
attr:       指向线程属性对象的指针。
注意事项:   销毁线程属性对象后，不应再使用它，除非重新初始化。用于释放任何与线程属性关联的资源。
*/

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
// detachstate:  可以是 PTHREAD_CREATE_DETACHED 或 PTHREAD_CREATE_JOINABLE。

int pthread_attr_getstack(const pthread_attr_t *attr, void **stackaddr, size_t *stacksize);
int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, size_t stacksize);
//注意事项:    这是高级用法，一般不推荐修改默认栈设置。

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);

int pthread_attr_getguardsize(const pthread_attr_t *attr, size_t *guardsize);
int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);
// 注意事项:    设置线程栈末尾的保护区大小。较大的保护区可以更好地防止栈溢出，但会使用更多的内存。

int pthread_attr_getscope(const pthread_attr_t *attr, int *scope);
int pthread_attr_setscope(pthread_attr_t *attr, int scope);
// 可能的值为 PTHREAD_SCOPE_SYSTEM 或 PTHREAD_SCOPE_PROCESS。

int pthread_attr_getschedpolicy(const pthread_attr_t *attr, int *policy);
int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy);
// policy:      调度策略，如 SCHED_FIFO, SCHED_RR, 或 SCHED_OTHER。
// 注意事项:    设置线程的调度策略。不同的策略影响线程的执行顺序和时间。
```

### 同步属性

#### 互斥量属性

```c++
// 关于pthread_mutexattr_t
int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);

int pthread_mutexattr_getpshared(const pthread_mutexattr_t *attr, int *pshared);
int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared);
// PTHREAD_PROCESS_SHARED 或 PTHREAD_PROCESS_PRIVATE

int pthread_mutexattr_getrobust(const pthread_mutexattr_t *attr, int *robust);
int pthread_mutexattr_setrobust(pthread_mutexattr_t *attr, int robust);
// 设置互斥锁的健壮性属性，以指定当拥有互斥锁的线程异常终止时的行为。PTHREAD_MUTEX_ROBUST 允许另一个线程恢复这个互斥锁，而 PTHREAD_MUTEX_STALLED 则不允许恢复，可能导致死锁。

int pthread_mutex_consistent(pthread_mutex_t *mutex);
// 注意事项:   当一个线程持有健壮型互斥锁 PTHREAD_MUTEX_ROBUST 且该线程终止时，其他线程可以锁定该互斥锁，但在使用之前必须调用此函数使互斥锁恢复一致状态。只有在锁处于不一致状态时调用此函数才是合法的。


int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
/*
attr:       指向互斥锁属性对象的指针。
type:       指定新的互斥锁类型。常用的类型包括：
            PTHREAD_MUTEX_NORMAL,
            PTHREAD_MUTEX_ERRORCHECK,
            PTHREAD_MUTEX_RECURSIVE。

注意事项:   设置互斥锁的类型可以改变互斥锁的行为。例如，PTHREAD_MUTEX_RECURSIVE 允许同一个线程多次锁定同一个互斥锁。
*/
```

#### 读写锁属性

```c++
int pthread_rwlockattr_init(pthread_rwlockattr_t *attr);
int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr);

int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr, int pshared);
int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *attr, int *pshared);


int pthread_rwlockattr_setkind_np(pthread_rwlockattr_t *attr, int pref);

/*
attr:       指向读写锁属性对象的指针。
pref:       指定锁的类型，影响读写操作的优先级。常见的类型包括：
            PTHREAD_RWLOCK_PREFER_READER_NP,
            PTHREAD_RWLOCK_PREFER_WRITER_NP,
            PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP.

注意事项:   选择合适的类型可以防止读者或写者饥饿，特别是在高负载条件下。
*/

int pthread_rwlockattr_getkind_np(const pthread_rwlockattr_t *attr, int *pref);
```

#### 条件变量属性

```c++
int pthread_condattr_init(pthread_condattr_t *attr);
int pthread_condattr_destroy(pthread_condattr_t *attr);
int pthread_condattr_getpshared(const pthread_condattr_t *attr, int *pshared);
int pthread_condattr_setpshared(pthread_condattr_t *attr, int pshared);
```

#### 屏障属性

```c++
int pthread_barrierattr_init(pthread_barrierattr_t *attr);
int pthread_barrierattr_destroy(pthread_barrierattr_t *attr);
int pthread_barrierattr_getpshared(const pthread_barrierattr_t *attr, int *pshared);
int pthread_barrierattr_setpshared(pthread_barrierattr_t *attr, int pshared);
```

### 线程特定数据

```c++
int pthread_key_create(pthread_key_t *key, void (*destructor)(void*));

/*
key:         指向线程特定数据键的指针。
destructor:  当线程终止时调用的析构函数，用于清理线程特定数据。如果不需要自动清理，则可以设置为 NULL。

注意事项:    此函数用于创建一个线程特定数据键。每个线程可以使用该键存储和访问其自己的数据实例。
*/

int pthread_key_delete(pthread_key_t key);

/*
key:        线程特定数据键。

注意事项:    删除键后，与键关联的数据不再可访问。如果键拥有一个析构函数，析构函数将不会在此函数调用时被触发。
*/


int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

/*
once_control:   指向 pthread_once_t 类型的静态或动态分配的变量，用于控制 init_routine 是否已执行。
init_routine:   指定的初始化函数，该函数在程序执行期间只执行一次。

注意事项:    通常用于初始化线程特定数据键或其他只需初始化一次的资源。
*/


void *pthread_getspecific(pthread_key_t key);

/*
key:        线程特定数据键。

注意事项:    返回与键关联的线程特定数据的值。如果没有为该键设置值，则返回 NULL。
*/

int pthread_setspecific(pthread_key_t key, const void *value);

/*
key:        线程特定数据键。
value:      要与键关联的数据值。

注意事项:    设置与键关联的线程特定数据的值。每个线程可以为同一个键关联其独特的值。
*/
```

### 线程和信号

```c++
int pthread_sigmask(int how, const sigset_t *set, sigset_t *oldset);

/*
how:        指定如何修改当前线程的信号掩码。可以是 SIG_BLOCK (添加 set 指定的信号到当前掩码),
            SIG_UNBLOCK (从当前掩码中移除 set 指定的信号), 或 SIG_SETMASK (将当前掩码设置为 set 指定的值)。
set:        指向信号集的指针，这些信号需要根据 how 参数进行阻塞、解阻或替换。
oldset:     可选参数，如果非 NULL，当前信号掩码将被保存在 oldset 中。

注意事项:   用于更改或检查调用线程的信号掩码。不影响其他线程。用于阻止或允许特定信号到达线程，通常用于同步信号处理。
*/

int sigwait(const sigset_t *set, int *sig);

/*
set:        指向信号集的指针，这个集合中包含了线程希望等待的信号。
sig:        指向整型的指针，用于存储被捕获信号的编号。

注意事项:   线程调用 sigwait 并阻塞，直到 set 参数指定的信号集中的任意信号被递送。递送的信号编号会存储在 sig 参数指向的位置。这个函数用于同步信号处理，特别是当你希望信号像常规事件一样处理时。
*/

int pthread_kill(pthread_t thread, int sig);

/*
thread:     目标线程的标识符。
sig:        要发送到目标线程的信号编号。如果 sig 为 0，则不发送信号，但系统会进行错误检查。

注意事项:   用于向指定线程发送信号。如果 sig 不为 0，相应的信号会被发送到指定的线程。如果线程已经设置了对该信号的阻塞，信号将保持挂起状态直到线程解除阻塞。
*/

```

### 线程和 **Fork**

```c++
int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));

/*
prepare:    这个函数在fork()调用之前，在父进程中执行。用于准备fork，通常是为了锁定所有需要在fork时保持一致性的资源。
parent:     这个函数在fork()调用之后，在父进程中执行。用于清理prepare阶段可能采取的任何行动，如解锁之前锁定的资源。
child:      这个函数在fork()调用之后，在子进程中执行。它通常用于重新初始化子进程状态，清理继承自父进程的资源锁定状态等。

注意事项:   pthread_atfork用于注册在fork调用过程中需要执行的处理函数。这是必要的，因为fork在多线程环境中的行为可能会导致问题，如锁状态在子进程中被复制，可能导致死锁。
*/

```

## 高级 IO

### 记录锁

```c++
int flock(int fd, int operation);
// flock() 函数用于在文件上应用一个劝告性锁。它可以锁定文件的整个部分，而不是像 fcntl 那样可以对文件的特定部分加锁。
/*
    LOCK_SH: 设置共享锁。多个进程可以同时持有同一个文件的共享锁。
    LOCK_EX: 设置排他锁。只有一个进程可以持有排他锁。
    LOCK_UN: 解除锁定。
    LOCK_NB: 可以与 LOCK_SH 或 LOCK_EX 结合使用，使操作变为非阻塞模式。
*/

struct flock {
    short l_type;   // 锁的类型: F_RDLCK, F_WRLCK, 或 F_UNLCK
    short l_whence; // 偏移量的起点: SEEK_SET, SEEK_CUR, 或 SEEK_END
    off_t l_start;  // 相对于 l_whence 的偏移量，锁定的起始点
    off_t l_len;    // 要锁定的字节数；0 表示从 l_start 到文件末尾
    pid_t l_pid;    // 持有锁的进程ID（仅在 F_GETLK 命令中返回）
};
```

### IO多路复用

#### ``select`` ``pselect``

```c++
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

/*
nfds:       监听的文件描述符数量，通常设置为最大文件描述符值加1。
readfds:    监听读操作的文件描述符集合。
writefds:   监听写操作的文件描述符集合。
exceptfds:  监听异常情况的文件描述符集合。
timeout:    指定等待时间，如果设置为 NULL，则无限等待；否则在 timeout 指定的时间后超时。

注意事项:   select 可能会修改 timeout 的值，所以每次调用前都需要重新设置。由于 select 使用的 fd_set 结构有大小限制，通常不能处理超过 1024 的文件描述符。
*/

int FD_ISSET(int fd, fd_set *fdset);
void FD_CLR(int fd, fd_set *fdset);
void FD_SET(int fd, fd_st *fdset);
void FD_ZERO(fd_set *fdset);
```

```c++
int pselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timespec *timeout, const sigset_t *sigmask);

/*
nfds:       监听的文件描述符的最大值加1。
readfds:    监听读操作的文件描述符集合。
writefds:   监听写操作的文件描述符集合。
exceptfds:  监听异常情况的文件描述符集合。
timeout:    指定等待时间，使用 timespec 结构表示，提供更高精度的时间控制。
sigmask:    一个信号掩码，用于在 pselect 调用期间替换进程的当前信号掩码。

注意事项:   pselect 不修改 timeout 参数，提供了与信号原子性处理的能力，从而避免了 select 可能遇到的某些竞争条件。
*/
```

#### ``poll``

```c++
int poll(struct pollfd *fds, nfds_t nfds, int timeout);

/*
fds:        指向 pollfd 结构数组的指针，每个元素包含一个文件描述符和要监听的事件以及发生的事件。
nfds:       fds 数组中的元素数量。
timeout:    超时时间，单位为毫秒。如果为 -1，则无限等待；如果为 0，则立即返回，即使没有文件描述符就绪。

注意事项:   poll 没有最大文件描述符数量的限制，适用于需要监视大量文件描述符的场合。
*/

struct pollfd{
    int fd;             /* fd to check, or < 0 to ignore */
    short events;       /* events of interest on fd */
    short revents;      /* events that occurred on fd */
}

/*
    POLLIN: 数据可读。
    POLLOUT: 数据可写。
    POLLPRI: 有紧急数据可读。
    POLLERR: 发生错误。
    POLLHUP: 设备挂断。
    POLLNVAL: 指定的文件描述符不是一个打开的文件。
*/
```

#### ``epoll``

```c++
int epoll_create(int size);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

/*
epoll_create:
    size:   建议的大小，不过这个参数现在不是非常关键，只需要是一个正数即可。

epoll_ctl:
    epfd:   由 epoll_create 返回的文件描述符。
    op:     控制操作，如 EPOLL_CTL_ADD, EPOLL_CTL_MOD 或 EPOLL_CTL_DEL。
    fd:     要操作的文件描述符。
    event:  指向 epoll_event 结构的指针，指定事件。

epoll_wait:
    epfd:       由 epoll_create 返回的文件描述符。
    events:     用于返回已就绪事件的 epoll_event 结构数组。
    maxevents:  events 数组的最大元素数量。
    timeout:    超时时间，单位为毫秒。如果为 -1，则无限等待；如果为 0，则立即返回，即使没有事件也是如此。

注意事项:   epoll 提供了高效的文件描述符事件监测机制，特别适用于高并发情况。它只在 Linux 系统上可用。
*/

struct epoll_event {
    uint32_t events;    /* Epoll events */
    epoll_data_t data;  /* User data variable */
};

typedef union epoll_data {
    void *ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;

/*
    EPOLLIN: 文件描述符可读（包括对端SOCKET正常关闭）。
    EPOLLOUT: 文件描述符可写。
    EPOLLRDHUP: 流套接字对端关闭连接，或关闭写半部分。
    EPOLLPRI: 有紧急数据可读（如TCP的带外数据）。
    EPOLLERR: 发生错误。
    EPOLLHUP: 发生挂断。然而，当对端正常关闭时，不一定能触发此事件。
    EPOLLET: 将 EPOLL 设为边缘触发(Edge Triggered)模式，这是相对于水平触发(Level Triggered)的一种触发方式。
    EPOLLONESHOT: 只监听一次事件，当监听到该事件后，如果需要再次监听，需要再次将文件描述符加入到 EPOLL 队列。
*/
```

### 异步 IO

#### System V

#### BSD

#### POSIX

```c++
// 异步IO控制块，包含了控制异步操作所需的所有信息
struct aiocb {
    int aio_fildes;     /* 文件描述符 */
    off_t aio_offset;   /* 文件偏移 */
    volatile void *aio_buf; /* 数据缓冲区 */
    size_t aio_nbytes;  /* 操作的字节数 */
    int aio_reqprio;    /* 请求的优先级 */
    struct sigevent aio_sigevent; /* 异步通知 */
    int aio_lio_opcode; /* 操作码，用于 lio_listio() */
};

// 用于指定异步IO完成时的通知方式
struct sigevent {
    int sigev_notify;             /* 通知类型 */
    int sigev_signo;              /* 信号 */
    union sigval sigev_value;     /* 信号传递的数据 */
    void (*sigev_notify_function)(union sigval); /* 回调函数 */
    pthread_attr_t *sigev_notify_attributes; /* 线程属性 */
};

/*
sigev_notify:
    SIGEV_NONE
        含义：无通知。使用这个选项时，不会有任何形式的通知发送给应用程序。这通常用于那些只需要检查异步操作状态而不需要立即通知的场景。
        用途：用于那些通过其他方式（如轮询）来检查操作完成状态的应用程序。
    SIGEV_SIGNAL
        含义：通过信号通知。当事件发生时，系统将向进程发送一个预定义的信号。
        用途：适合需要即时响应的场合，可以快速通知应用程序一个操作已经完成或者定时器已经到期。应用程序需要设置好信号处理函数来响应信号。
        参数：sigev_signo 指定要发送的信号类型，sigev_value 可以提供一个用户定义的数据值（通过信号处理函数的 si_value 访问）。
    SIGEV_THREAD
        含义：通过启动一个新线程来处理通知。当事件发生时，系统将创建一个新线程来运行一个由用户定义的函数。
        用途：适合需要执行较长时间或复杂处理的任务的场合，因为这种处理不会阻塞主程序的执行。
        参数：sigev_notify_function 指定新线程将调用的函数，sigev_notify_attributes 指定新线程的属性（如优先级），sigev_value 提供传递给线程函数的数据。
*/

union sigval {
    int sival_int;         /* Integer signal value */
    void *sival_ptr;       /* Pointer signal value */
};

int aio_read(struct aiocb *aiocbp);

/*
aiocbp:    指向初始化好的 aiocb 结构的指针，指定读操作的详细信息。

注意事项:  函数立即返回，读操作完成后通过结构中定义的方式通知。
*/

int aio_write(struct aiocb *aiocbp);

/*
aiocbp:    指向初始化好的 aiocb 结构的指针，指定写操作的详细信息。

注意事项:  函数立即返回，写操作完成后通过结构中定义的方式通知。
*/

int aio_fsync(int op, struct aiocb *aiocbp);    // 异步方式同步文件状态

/*
op:        同步操作的类型，O_SYNC 或 O_DSYNC。
aiocbp:    指向与文件描述符关联的 aiocb 结构。

注意事项:  用于将文件数据和状态同步到磁盘，操作完成后通过 aiocb 指定的方式通知。
*/

int aio_error(const struct aiocb *aiocbp);  // 获取异步操作的状态

/*
aiocbp:    指向 aiocb 结构的指针，该结构描述了一个异步操作。

注意事项:  返回当前异步操作的状态。如果操作仍在进行中，返回 EINPROGRESS；如果完成，返回 0；如果发生错误，返回具体错误代码。
*/

ssize_t aio_return(struct aiocb *aiocbp); // 或取异步操作的结果

/*
aiocbp:    指向 aiocb 结构的指针。

注意事项:  用于获取异步操作的最终结果，只应在操作完成后调用。
*/

int aio_suspend(const struct aiocb *const aiocbp[], int n, const struct timespec *timeout); // 等待一个或多个异步 I/O 操作完成。

/*
aiocbp:    指向 aiocb 结构指针数组，数组中每个元素都指向一个异步操作。
n:         数组中指针的数量。
timeout:   指定等待异步操作完成的最大时间。

注意事项:  如果在指定时间内异步操作未完成，函数返回错误。
*/

int aio_cancel(int fd, struct aiocb *aiocbp);   // 取消异步IO操作

/*
fd:        文件描述符。
aiocbp:    可选，指向一个 aiocb 结构的指针。如果为 NULL，取消所有与文件描述符关联的异步操作。

注意事项:  尝试取消一个或所有与文件描述符关联的异步操作，操作的取消可能不会立即生效。
*/

int lio_listio(int mode, struct aiocb *const list[], int nent, struct sigevent *sig);   // 启动一组异步IO操作

/*
mode:      操作模式，LIO_WAIT (等待所有操作完成) 或 LIO_NOWAIT (立即返回)。
list:      指向 aiocb 指针数组，每个指针代表一个 I/O 操作。
nent:      数组中的元素数量。
sig:       指定异步操作完成时如何通知进程。

注意事项:  用于同时发起多个读写操作，可通过 sig 指定通知方式。
*/
```

### ``readv`` ``writev``

```c++
#include <sys/uio.h>

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);

/*
fd:       文件描述符。
iov:      指向多个缓冲区的指针数组，这些缓冲区用于存储读取的数据。
iovcnt:   缓冲区的数量。

注意事项: readv 会按照 iov 指定的缓冲区顺序来存储从 fd 读取的数据。返回值是读取的总字节数，如果到达文件末尾，则为0。
*/

ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

/*
fd:       文件描述符。
iov:      指向多个缓冲区的指针数组，这些缓冲区包含了要写入的数据。
iovcnt:   缓冲区的数量。

注意事项: writev 会按照 iov 指定的缓冲区顺序将数据写入到 fd 中。返回值是写入的总字节数。
*/

struct iovec {
    void *iov_base;
    size_t iov_len;
};

```

### 存储映射 IO

```c++
#include <sys/mman.h>

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);

/*
addr:     建议使用的映射起始地址，通常为 NULL，表示由系统决定映射区域。
length:   映射区域的长度。
prot:     映射区域的保护权限，可以是 PROT_READ, PROT_WRITE, PROT_EXEC, PROT_NONE 的组合。权限不能超过文件打开时候的权限
flags:    影响映射区域的各种标志，如 MAP_PRIVATE 或 MAP_SHARED。
fd:       文件描述符，其内容被映射到内存中。
offset:   文件中的偏移，从该位置开始映射。

注意事项: 使用 mmap 创建的映射在进程间是共享的，或者对于文件的修改是私有的，取决于 flags 参数。成功时返回指向映射区域的指针，失败时返回 MAP_FAILED。

MAP_PRIVATE
    定义：创建一个写时复制（copy-on-write）的内存映射。对这样的映射所做的修改不会反映到原始文件中，而是仅限于本地进程的复制。
    使用场景：适用于需要读取操作和可能修改数据但不想这些修改影响原文件的场景。
MAP_SHARED
    定义：创建一个共享内存映射。对映射区域的修改会反映到映射的文件上。
    使用场景：适用于多个进程需要访问和修改同一文件数据，并且希望这些修改对所有进程都是可见的情况。
*/

int munmap(void *addr, size_t length);

/*
addr:     mmap 返回的区域指针。
length:   要取消映射的区域长度。

注意事项: 成功调用 munmap 后，addr 到 addr + length 的区域不再有效。返回 0 表示成功，-1 表示失败。
*/

int mprotect(void *addr, size_t len, int prot); // 用于改变一段内存区域的保护属性，可以控制程序对这段内存的访问权限。
int msync(void *addr, size_t length, int flags); // 于将内存映射文件的一部分或全部数据同步到磁盘文件中，确保文件数据的一致性。

/*
    addr: 指向需要同步到磁盘的内存区域的起始地址。
    length: 需要同步的内存区域的长度。
    flags: 控制同步操作的方式。常用的标志包括：
        MS_ASYNC: 异步同步，只是将页面标记为需要同步，函数立即返回。
        MS_SYNC: 同步同步，msync() 在所有数据都写到磁盘之前不返回。
        MS_INVALIDATE: 在同步后，使内核中的页面缓存无效。
*/

```

## 进程间通信

### 管道

```c++
int pipe(int filedes[2]);
/*
filedes:   一个整型数组，pipe函数通过这个参数返回两个文件描述符：filedes[0] 用于读，filedes[1] 用于写。

注意事项:  pipe是最基本的IPC方式之一，用于在有血缘关系的进程之间进行通信。创建成功后，数据可以从一个进程流向另一个进程。
*/
```

### popen

```c++
FILE *popen(const char *command, const char *type);

/*
command:   要执行的命令。
type:      类型，"r" 表示读取，"w" 表示写入。

注意事项:  popen允许执行一个命令，并读取或写入该命令的标准输出或标准输入。使用完后应通过pclose关闭。
*/

int pclose(FILE *stream);

/*
stream:    由popen返回的文件指针。

注意事项:  pclose关闭popen打开的流，并等待命令执行完毕，返回命令的退出状态。
```

### FIFO

```c++
int mkfifo(const char *pathname, mode_t mode);

/*
pathname:  FIFO文件的路径。
mode:      设置文件的权限。

注意事项:  创建一个命名管道，用于非血缘关系进程间的通信。
*/

int mkfifoat(int dirfd, const char *pathname, mode_t mode);

/*
dirfd:     相对目录的文件描述符。
pathname:  FIFO文件的路径。
mode:      设置文件的权限。

注意事项:  功能类似mkfifo，但可以相对于一个打开的目录文件描述符创建FIFO。
*/
```

### XSI IPC

```c++
key_t ftok(const char *pathname, int proj_id);

/*
pathname:  任何存在的文件路径，通常是一个普通文件或目录。
proj_id:   一个非零字符，确保在同一路径下生成不同的键。

注意事项:  生成一个System V IPC键，通常用于shmget或semget。
*/
```

### 消息队列

```c++
int msgget(key_t key, int msgflg);

/*
key:       消息队列的键，通常由ftok生成。
msgflg:    消息队列的创建标志和权限。

注意事项:  用于创建或访问一个消息队列。
*/

int msgctl(int msqid, int cmd, struct msqid_ds *buf);

int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);

/*
msqid:     消息队列标识符，由msgget返回。
msgp:      指向准备发送的消息的指针。
msgsz:     消息数据部分的长度，不包括消息类型字段。
msgflg:    操作标志，如IPC_NOWAIT（非阻塞发送）。

注意事项:  将消息添加到消息队列。如果队列已满，根据msgflg的设置，调用可能阻塞或非阻塞。
*/

ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);

/*
msqid:     消息队列标识符，由msgget返回。
msgp:      指向接收消息的缓冲区的指针。
msgsz:     消息数据部分的最大长度。
msgtyp:    接收消息的类型：
             0: 接收队列中的第一条消息。
             >0: 接收给定类型的第一条消息。
             <0: 接收第一个键值<=msgtyp绝对值的消息。
msgflg:    操作标志，如IPC_NOWAIT（非阻塞接收），MSG_EXCEPT，MSG_NOERROR。

注意事项:  从队列中接收消息。根据msgtyp的值，可以有选择地接收。
*/
```

### 信号量

```c++
int semget(key_t key, int nsems, int semflg);

/*
key:       信号量的键。
nsems:     信号量集中的信号量数目。
semflg:    创建标志和权限。

注意事项:  用于创建或访问一个信号量集。
*/

int semctl(int semid, int semnum, int cmd, ...);

/*
semid:     信号量集标识符。
semnum:    信号量集中的信号量索引。
cmd:       控制操作，如SETVAL（设置信号量值）、GETVAL（获取信号量值）、IPC_RMID（删除信号量集）等。
...:       额外的参数，取决于cmd的值。

注意事项:  对信号量进行直接控制。使用多功能取决于cmd参数的设置。
*/

int semop(int semid, struct sembuf *sops, size_t nsops);

/*
semid:     信号量集标识符。
sops:      指向sembuf结构数组的指针，每个结构描述一个操作。
nsops:     操作的数量。

注意事项:  对信号量执行操作数组。每个操作可以增加、减少或等待信号量值变为零。
*/
```

### 共享存储

```c++
int shmget(key_t key, size_t size, int shmflg);

/*
key:       共享内存段的键。
size:      共享内存段的大小。
shmflg:    创建标志和权限。

注意事项:  用于创建或访问一个共享内存段。
*/

int shmctl(int shmid, int cmd, struct shmid_ds *buf);

/*
shmid:     共享内存标识符。
cmd:       控制操作，如IPC_STAT（获取共享内存状态）、IPC_SET（设置共享内存参数）、IPC_RMID（删除共享内存段）等。
buf:       指向shmid_ds结构的指针，用于获取或设置共享内存的属性。

注意事项:  对共享内存段进行控制。使用多功能取决于cmd参数的设置。
*/

void *shmat(int shmid, const void *shmaddr, int shmflg);

/*
shmid:     共享内存标识符。
shmaddr:   指定共享内存连接到进程的地址空间的地址，通常为NULL，表示系统选择地址。
shmflg:    操作标志，如SHM_RDONLY（只读连接）。

注意事项:  将共享内存段附加到进程的地址空间。返回指向共享内存的指针。
*/

int shmdt(const void *shmaddr);

/*
shmaddr:   共享内存段附加到进程的地址空间的地址。

注意事项:  将共享内存段与进程的地址空间分离。成功时返回0，失败时返回-1。
*/

```

### POSIX 信号量

```c++
sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned int value);

/*
name:   信号量的名称，通常以前缀 '/' 开头。
oflag:  控制操作的标志，可能的值包括 O_CREAT（创建信号量，如果已存在则打开）、O_EXCL（与O_CREAT一起使用，确保创建新信号量）等。
mode:   设置新创建的信号量的权限（如果使用了O_CREAT）。
value:  信号量的初始值（如果使用了O_CREAT）。

注意事项:  创建或打开一个命名信号量。返回一个指向信号量对象的指针，或在错误时返回SEM_FAILED。
*/

int sem_close(sem_t *sem);

/*
sem:    指向打开信号量的指针。

注意事项:  关闭信号量。信号量名称仍存在，但其链接计数减少。
*/

int sem_unlink(const char *name);

/*
name:   信号量的名称。

注意事项:  删除信号量的名称。如果没有其他进程打开此信号量，它将被销毁。
*/

int sem_wait(sem_t *sem);

/*
sem:    指向信号量对象的指针。

注意事项:  对信号量执行P操作（减少）。如果信号量的值为零，则调用线程将阻塞，直到信号量可用。
*/

int sem_trywait(sem_t *sem);

/*
sem:    指向信号量对象的指针。

注意事项:  尝试对信号量执行P操作，如果信号量不可用，则不阻塞，直接返回错误。
*/

int sem_post(sem_t *sem);

/*
sem:    指向信号量对象的指针。

注意事项:  对信号量执行V操作（增加）。增加信号量的值，如果有线程在此信号量上阻塞，则至少有一个将被唤醒。
*/

int sem_getvalue(sem_t *sem, int *sval);

/*
sem:    指向信号量对象的指针。
sval:   用于存储信号量当前值的整数指针。

注意事项:  获取信号量的当前值。不应用于同步目的，因为值在返回时可能已经变化。
```

## 网路IPC

## 高级IPC

## 终端IO
