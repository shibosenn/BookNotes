# Effective Morden C++

## 类别推导

### 条款1：理解模板型别推导

### 条款2：理解 `auto` 型别推导

### 条款3：理解 `decltype`

### 条款4：掌握查看型别推导结构的方法

## `auto`

### 条款5：优先考虑 ``auto`` 而非显式类型声明

```c++
// 避免一些错误的写法
std::unordered_map<std::string, int> um;

for(const std::pair<std::string, int>& ele : um) {
    // 会调用拷贝构造生成一个临时对象给ele
    ...
}


// 提高重构效率
```

### 条款6：`auto` 推导若非己愿，使用显式类型初始化惯用法

避免隐形代理类型导致意外中的行为

```c++
std::vector<bool> vBool{true, false, true};
auto flag = vBool[0]; // type of flag: std::vector<bool>::reference
flag = false; // vBool[0] = false;
```

## 转向现代 `c++`

### 条款7：区别使用 `( )` 和 ``{ }`` 创建对象

- 大括号对于解析语法“免疫”

    ```c++
    class Widget {
    private:
        int x = 0;
        int y = (1);
        // int y (1); ERROR, 会导致编译歧义为函数声明，
        int z {2};
        int w = {2};
    };
    // 声明时初始化会被初始化列表覆盖
    ```

- 大括号初始化的优先级

    - 在构造函数被调用时，如果形参中没有任何一个具备 `std::initializer_list` 类别，那么小括号和大括号的意义就没有区别
    - 编译器调用具备`std::initializer_list` 型参的构造函数的意愿非常强烈 **！**

        ````c++
        class Widget {
        public:

            Widget(int x, bool y) { } // 1

            Widget(std::initializer_list<std::int> iniL) { } // 2

            operator float() { }
        };

        Widget a(10, true); // 调用 1
        Widget b{10, true}; // 调用 2
        // 如果参数写成 std::initializer_list<std::bool> 还会导致编译错误
        // 如果写成 std::initializer_list<std::string> 会调用 1
        
        Widget c{b}; // 有时会调用2
        ````

- 在模板内容进行对象创建时，到底应该使用小括号还是大括号会成为一个棘手问题

### 条款8：优先考虑`nullptr`

### 条款9：优先考虑别名声明而非``typedefs``

### 条款10：优先考虑限域``enum``而非未限域`enum`

- 限定作用域的枚举型别的默认底层型别是 ``int`` , 而不限范围的枚举型别没有默认底层型别
- 限定作用域的枚举型别总是可以进行前置声明，不限范围的枚举型别却只有在指定了默认底层型别的前提下可以进行前置声明

### 条款11：优先考虑使用 `deleted` 函数而非使用未定义的私有声明

- 通过 ``delete`` 删除重载版本

    ```c++
    bool isLucky(int);
    bool isLucky(double) = delete;
    bool isLucky(bool) = delete;
    ```

- 阻止模板具现

    ```c++
    template<typename T>
    void processPointer(T* ptr);

    template<>
    void processPointer(void*) = delete;

    template<>
    void processPointer(char*) = delete;
    ```

### 条款12：使用 ``override`` 声明重写函数

- 引用饰词

    ```c++
    class Widget {
    public:
        using DataType = std::vector<A>;
        ...
        DataType& getData() & {
            return values;
        }
        DataType&& getData() && {
            return std::move(values);
        }
    private:
        DataType values;
    }w;

    auto values1 = w.getData();
    auto values2 = std::move(w).getData();
    ```

### 条款13：优先考虑 `const_iterator` 而非 ``iterator``

### 条款14：如果函数不抛出异常请使用 `noexcept`

- 尽量让构造函数（ 普通，赋值、移动 ） `noexcept`

    ```c++
    class A {
    public:
        A() { }
        A(const A& other) { }
        A(A&& other) noexcept { } // 这里的标记会带来效率上的提升，比如 push_back 会调用 noexcept 的移动语义

        // std::vector::push_back 会调用 std::move_if_noexcept进行处理
    };    
    ```

- 尽量让 `swap` 函数声明为 ``noexcept``

    ```c++
    template<class T, size_t N>
    void swap(T (&a)[N], T (&b)[N]) noexcept(noexcept(swap(*a, *b)));

    // 标准库中swap是否带有noexcept声明，取决于用户定义的swap是否带有noexcept声明
    ```

### 条款15：尽可能的使用 `constexpr`

```c++
// 在 c++11 中通过三目运算符和递归实现更复杂的功能
constexpr int pow(int base, int exp) noexcept {
    return (exp == 0 ? 1 : base * pow(base, exp - 1));
}
```

### 条款16：让 ``const`` 成员函数线程安全

### 条款17：理解特殊成员函数的生成

- 声明移动构造函数会阻止生成移动赋值运算符，反之亦然。此规则对于复制方式不适用

- 移动操作生成条件：
    - 未声明任何复制操作
    - 未声明任何移动操作
    - 未声明任何析构操作

- 成员模板在任何情况下都不会抑制特种成员函数的生成

## 智能指针

### 条款18：对于独占资源使用 `std::unique_ptr`

- 析构器会影响指针的 `std::unique_ptr` 的大小

    ```c++
    auto lambda_deleter = [](int* p) { delete p; }; // sizeof(std::unique_ptr<int, decltype(lambda_deleter)>) = 8

    // 通过 + 把无状态 lambad 转化为函数指针
    auto function_deleter = +[](int* p) { delete p; }; // sizeof(std::unique_ptr<int, decltype(function_deleter)>) = 16

    struct StatefulDeleter {
        int state = 0;
        void operator()(int* p) { delete p; }
    };

    // sizeof(std::unique_ptr<int, StatefulDeleter>) = 16
    ```

### 条款19：对于共享资源使用 `std::shared_ptr`

- 引用计数机制的性能代价
    - ``std::shared_ptr``的尺寸是裸指针的两倍
    - 引用计数的内存必须动态分配
    - 引用计数的增减操作必须是原子操作，会带来更大的开销

### 条款20：当 `std::shared_ptr` 可能悬空时使用 `std::weak_ptr`

### 条款21：优先考虑使用 ``std::make_unique`` 和 ``std::make_shared`` 而非直接使用 ``new``

- 不适合使用 ``make_shared`` 的情况

    - 需要自定义删除器
    - 需要使用 ``{}`` 初始化
    - 大对象的情况，对象会在引用计数变成0的时候析构，但是托管对象所占用的内存会在与其关联的控制块也被析构的时候才会释放

### 条款22：当使用 ``Pimpl`` 惯用法，请在实现文件中定义特殊成员函数

## 左值引用、移动语义和完美转发

### 条款23：理解 ``std::move`` 和 `std::forward`

### 条款24：区分通用引用与右值引用

### 条款25：对右值引用使用 ``std::move``，对通用引用使用 ``std::forward``

### 条款26：避免在通用引用上重载

### 条款27：熟悉通用引用重载的替代方法

### 条款28：理解引用折叠

### 条款29：假定移动操作不存在，成本高，未被使用

### 条款30：熟悉完美转发失败的情况

## ``lambda``

### 条款31：避免使用默认捕获模式

### 条款32：使用初始化捕获来移动对象到闭包中

### 条款33：对 `auto&&` 形参使用 ``decltype`` 以 ``std::forward`` 它们

### 条款34：考虑 `lambda` 而非 `std::bind`

## 并发API

### 条款35：优先考虑基于任务的编程而非基于线程的编程

### 条款36：如果有异步的必要请指定 ``std::launch::async``

### 条款37：使 ``std::thread`` 在所有路径最后都不可结合

### 条款38：关注不同线程句柄的析构行为

### 条款39：对于一次性事件通信考虑使用 ``void`` 的 ``futures``

### 条款40：对于并发使用 ``std::atomic`` ，对于特殊内存使用 ``volatile``

## 微调

### 条款41：对于移动成本低且总是被拷贝的可拷贝形参，考虑按值传递

### 条款42：考虑使用置入代替插入
