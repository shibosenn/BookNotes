# SIG STL源码分析

## STL前期准备

在学习STL源码之前需要对`template`有一个认识或者回忆.

### template

* 类模板的成员函数通常在实例化时才会被编译器生成相应的函数体，但并不是每个成员函数都一定会被实例化，而是只有在需要该成员函数的时候才会被实例化。这种特性被称为「延迟实例化」或「按需实例化」。
* 可以把类模板和函数模板结合起来, 定义一个含有成员函数模板的类模板
* 类模板中可以声明static成员, 在类外定义的时候要增加template相关的关键词, 并且需要注意每个不同的模板实例都会有一个独有的static成员对象.

    ```c++
    template<class T>
    class tmp {
    public:
        static T t;
    };

    template<class T>
    T tmp<T>::t = 0;

    int main() {
        tmp<int> t1;
        tmp<int> t2;
        tmp<double> t3;
        t1.t = 1;
        std::cout << "t1.t = " << t1.t << endl;
        std::cout << "t2.t = " << t2.t << endl;
        cout << "t3.t = " << t3.t << endl;
        return 0;
    }
    ```

* 非类型模版参数

    ```c++
    template<class T, std::size_t N>    // 这里的N是编译时期就知道了, 所以可以加上constexpr关键字
    constexpr std::size_t ArrSize(T (&a)[N]) noexcept
    {
        return N;
    }
    int a[100]; 
    int main() {      
        int size = ArrSize(a);
        cout << size << endl;
        return 0;
    }

    ```

## 空间配置器

### new

* operator new

    是一个可以被自定义实现的函数，它用于分配未初始化的内存块。

    ```c++
    #include <iostream>
    #include <cstdlib>

    void* operator new(size_t size) {
        std::cout << "Allocating " << size << " bytes." << std::endl;
        void* ptr = std::malloc(size);
        if (!ptr) {
            throw std::bad_alloc();
        }
        return ptr;
    }

    int main() {
        int* ptr = new int;
        delete ptr;
        return 0;
    }
    ```

* new operator
  
    是一个运算符，它用于创建对象并返回指向该对象的指针。它通过使用 operator new 来分配内存，并在内存上调用对象的构造函数
* placement new

    在已经申请的内存上构建对象

### allocator

```c++
const int N = 4;
int main()
{
    allocator<string>  alloc;
    auto str_ve = alloc.allocate(N);
    auto p = str_ve;
    alloc.construct(p++);
    alloc.construct(p++, 10, 'a');
    alloc.construct(p++, "construct");
    cout << str_ve[0] << endl;
    cout << str_ve[1] << endl;
    cout << str_ve[2] << endl;

    while(p != str_ve)
    {
        alloc.destroy(--p);
    }
    alloc.deallocate(str_ve, N);

    exit(0);
}

```

[2. 第一级配置器](https://blog.csdn.net/Function_Dou/article/details/84631393)

[3. 第二级配置器](https://blog.csdn.net/Function_Dou/article/details/84631714)

[4. 内存池](https://blog.csdn.net/Function_Dou/article/details/84632272)

### __gun_cxx空间中的分配器

#### [bitmap_allocator](https://www.cnblogs.com/hjx168/p/16094936.html)

`bitmap_allocator` 的基本思想是使用一个位图来表示内存中的空闲块和已分配的块，这样就可以快速查找和分配未使用的内存块，而不必遍历整个堆。当分配一个对象时，分配器会在位图上查找第一个空闲块，分配内存，然后设置相应的位图项以表示此块已被占用。同样，在释放对象时，分配器会将相应的位图项标记为未使用。

#### [pool_allocator](https://www.cnblogs.com/hjx168/p/16074184.html)

`pool_allocator` 是一种预分配内存池的分配器。它的基本思想是在使用容器之前分配一大块连续的内存，然后在容器中使用这个内存池分配元素，当元素不再使用时，将其释放回内存池，这些被释放的元素块可以被下次分配器直接使用，这样就避免了频繁的动态内存分配和释放操作。

使用 pool_allocator 可以显著提高容器的性能，并且**可以避免内存碎片化的问题**，因为元素是从连续的内存中分配的，并且不需要频繁地向操作系统请求新的内存块。

#### [mt_allocator](https://www.cnblogs.com/hjx168/p/16060182.html)

`mt allocator`是一种以2的幂次方字节大小为分配单位的空间配置器，支持多线程和单线程。该配置器灵活可调，性能高。

## 迭代器

每个组件都可能会涉及到对元素的简单操作, 比如 : 访问, 自增, 自减等. 每个组件都的数据类型可能不同, 所以每个组件可能都要自己设计对自己的操作. 但将每个组件的实现成统一的接口就是迭代器

它的优点也很明显:

1. 它是能屏蔽掉底层数据类型差异的.
2. 迭代器将容器和算法粘合在一起, 使版块之间更加的紧凑, 同时提高了执行效率, 让算法更加的得到优化.

这些实现大都通过`traits`编程实现的.  它的定义了一个类型名规则, 满足`traits`编程规则就可以自己实现对`STL`的扩展, 也体现了`STL`的灵活性. 同时`straits`编程让程序根据不同的参数类型选择执行更加合适参数类型的处理函数, 也就提高了`STL`的执行效率. 可见迭代器对`STL`的重要性.

### 迭代器类型及实例

* 输入/输出迭代器（Input/Output Iterator）

    从文件或标准输入/输出中读入读出数据，并且每个元素只能被遍历一次。

    ```c++

    #include <iostream>
    #include <vector>
    #include <iterator>
    #include <algorithm>

    int main()
    {
        std::istream_iterator<int> input(std::cin);

        std::vector<int> vec(input, std::istream_iterator<int>()); // 从标准输入读入一组整数

        std::sort(vec.begin(), vec.end()); // 对整数进行排序

        std::ostream_iterator<int> output(std::cout, " ");

        std::copy(vec.begin(), vec.end(), output); // 输出排序后的结果

        std::cout << std::endl;
    }
    ```

* 前向迭代器（Forward Iterator）

    前向迭代器允许遍历序列中的每个元素，并且每个元素可以被遍历一次或多次，但是不能进行反向遍历。在 STL 中，例如 `std::forward_list` 就是前向迭代器的一种实现。

* 双向迭代器（Bidirectional Iterator）

    双向迭代器允许遍历序列中的每个元素，并且每个元素可以被遍历一次或多次，但是不能进行随机访问。在 STL 中，例如 `std::list` 就是双向迭代器的一种实现。

* 随机访问迭代器（Random Access Iterator）

    随机访问迭代器允许对序列中的元素进行随机访问，并且支持算术操作，例如比较、加减等操作。在 STL 中，例如 `std::vector` 就是随机访问迭代器的一种实现。

> [迭代器通用实现](/STL/code/iterator.cpp)

**false_type**

```c++
template<typename T>
struct false_type : public integral_constant<bool, false> {};
```

## 容器

### 概述

![容器概述](/images/%E5%AE%B9%E5%99%A8%E6%A6%82%E8%BF%B0.png)
![容器迭代器类型](/images/%E5%AE%B9%E5%99%A8%E8%BF%AD%E4%BB%A3%E5%99%A8%E7%B1%BB%E5%9E%8B.png)

### [vector有序容器](/STL/STL-master/vector.md)

* 扩容原理
* `reserve` 和 `resize` 区别
* `emplace_pach` 和`push_back` 区别

### [list有序容器](/STL/STL-master/list.md)

### [deque有序容器](/STL/STL-master/deque.md)

### [stack适配器](/STL/STL-master/stack.md)

### [queue配接器](/STL/STL-master/queue.md)

### [heap大根堆](/STL/STL-master/heap.md)

### [优先级队列](/STL/STL-master/priority_queue.md)

### [RB-tree关联容器](/STL/STL-master/RB-tree.md)

### [set配接器](/STL/STL-master/set.md)

### [pair结构体](/STL/STL-master/pair.md)

### [map配接器](/STL/STL-master/map.md)

* map为什么使用红黑树实现，而不是用AVL
  * 红黑树的平衡调整操作相对较少，它们的时间复杂度较低。在搜索和插入操作中，它们的性能相对较好。同时，红黑树在实现中需要的操作也比较基础，没有其他平衡树复杂。

  * 另外，红黑树相对于 AVL 树对内存的要求更低，主要是因为 AVL 树对于平衡性的强要求使得其节点需要存储比较多的元数据，比如每个节点需要存储平衡因子，而平衡因子需要用一个字节或更大的空间存储，这将增加节点的大小，从而导致占用更多的内存空间。而红黑树相对较少的平衡要求使得节点的存储相对较少，只需要几个颜色位（通常用1比特表示），从而在节点大小上进行了优化，占用内存相对更少。

* `map[]`、`map.at()` 和 `map.find()` 的区别如下：

    * `map[]` 通过 key 直接访问 map 中与之关联的 value，并返回对应的值。如果 key 在 map 中不存在，则会创建一个默认值，并返回。这个操作如果添加了不存在的键，则会插入一组数据。
    * `map.at()` 与 map[key] 相似，通过 key 直接访问 map 中与之关联的值，但不同的是，如果 key 在 map 中不存在，则会抛出一个std::out_of_range异常。
    * `map.find()` 通过 key 查找 map 中与之关联的 value，并返回一个指向该值的 iterator，如果 key 在 map 中不存在，则返回末尾的 iterator。

### [multiset配接器](/STL/STL-master/multiset.md)

### [multimap配接器](/STL/STL-master/multimap.md)

### [hashtable关联容器](/STL/STL-master/hashtable.md)

### [hash_set配接器](/STl/STL-master/hash_set.md)

### [hash_multiset配接器](/STL/STL-master/hash_multiset.md)

### [hash_map配接器](/STL/STL-master/hash_map.md)

### [hash_multimap配接器](/STL/STL-master/hash_multimap.md)

### 算法

### 仿函数

### 配接器

## 参考

>[侯捷STL](/books/%E4%BE%AF%E6%8D%B7/%E4%BE%AF%E6%8D%B7STL%E6%A0%87%E5%87%86%E5%BA%93%E5%92%8C%E6%B3%9B%E5%9E%8B%E7%BC%96%E7%A8%8B/%E4%BE%AF%E6%8D%B7STL%E6%A0%87%E5%87%86%E5%BA%93%E5%92%8C%E6%B3%9B%E5%9E%8B%E7%BC%96%E7%A8%8B/Slide.pdf)
