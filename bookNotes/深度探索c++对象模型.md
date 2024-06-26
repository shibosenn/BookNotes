# 深度探索c++对象模型

## 关于对象

- c++ 在布局以及存取时间上主要的额外负担是由 `virtual` 引起

- 需要多少内存才能够表现一个 ``class object``

    - 非静态成员变量
    - 由于对齐产生的填补
    - 为了支持虚拟而产生的额外负担

- 指针的类型

    - “指向不同类型之各指针”间的差异，既不在其指针表示法不同，也不在其内容不同，而是在于寻址出来的 **对象** 类型不同
    - **指针类型教导编译器如何解释某个特定地址中的内存内容及其大小**
    - 一个类型为 ``void*`` 的指针只能够含有一个地址，而不能通过它操作所指之对象

- OO 编程需要有一个未知实体 -> 通过指针或者引用完成

    ```c++
    // 不确定类型–
    Librar_materials *px = retrieve_some_material();
    Librar_materials *rx = *px;

    // 确定类型
    Librar_materials dx = *px;
    ```

## 构造函数语义学

### Default Constructor 的建构操作

`nontrivial default constructor` 的四种情况：

> 注意：默认构造函数只是满足编译器的需要，而不满足程序员的需要

- 带有`Defalult Constructor` 的 `Member class object`
- 带有`Defalult Constructor` 的 `base class`
- 带有`virtual function` 的 `class`
- 带有`virtual base class` 的 `class`

### Copy Conctructor 的建构操作

## Data 语义学

## Function 语义学

## 构造、结构、拷贝语义学

## 执行期语义学

## 站在对象模型的类端
