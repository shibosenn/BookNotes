# Effective C++

## 0. 导读

### 运算符优先级

```c++
[] () . -> 						// 一级, 
- (type) ++ -- * & ! ~ sizeof 	// 二级  从右到左
* / % 							// 三级  
+ -   							// 四级  
>> <<
> >= < <=
== !=
&
^
|
&&
||
?:									// 	从右到左
= /= *= %= += -= <<= >>= &= ^= |=	//	从右到左
,
// ！ > 算术运算符 > 关系运算符 > && > || >赋值运算符

// [] 本质上是指针算数的语法糖
```

### 声明 定义

- 声明是告诉编译器某个变量或函数的存在，不会导致分配内存，可以出现多次

	```c++
	extern int x; // 声明

	class A{
		static const int x = 1; // 声明
		static int y; // 声明
		// 静态成员属于整个类，而不属于每个对象，如果在类内初始化，会导致每个对象都包含该静态成员，这是矛盾的
	};
	```

- 定义会导致分配内存，只能定义一次

    ```c++
    int x; // 定义
	int A::y = 1; // 定义
	// 在类外定义和初始化静态成员变量是保证静态成员变量只被定义一次的好方法

	const int A::x; // 定义
    ```

### 内部链接 外部链接

- 将具有外部链接的定义放在头文件中几乎都是编程错误。因为如果该头文件中被多个源文件包含，那么就会存在多个定义，链接时就会出错。

- 在头文件中放置内部链接的定义却是合法的，但不推荐使用的。因为头文件被包含到多个源文件中时，在每个编译单元中有自己的实体存在。大量消耗内存空间，还会影响机器性能。

### 初始化

- 默认构造函数 没有参数或者每个参数都有缺省值
- 区分 ``copy assignment`` 和 ``copy constructor``

    ```c++
    Widget w1, w3;
    Widget w2 = w1; // copy constructor 一个新对象被定义，一定会有构造函数被调用
    w3 = w1; // copy assignment
    ```

- ``copy constructor`` 定义了一个对象如何 `passed by value`
- 关于 拷贝省略

	```c++
	class Widget{
	public:
		Widget(int initialValue = 0) : value(initialValue) {
			...
		}
		Widget(const Widget& w) {
			...
		}
	private:
		int value;
	}

	Widget w1 = 1; // 这里其实只会调用默认构造函数进行原地构造，而不是生成临时对象在进行拷贝构造
	Widget w2, w3;
	w2 = 1; 		// 原地构造
	w3 = Widget(1)  // 原地构造 
	```

## 1. 让自己习惯 c++

### 条款02：尽量以 ``const`` ``enum`` ``inline`` 替换 `#define`

- `enum hack`

	```c++
	class GamePlayer{
		enum { NumTurns = 5; }
		int scores[NumTurns];
	};
	// 可以满足对编译时常量的需求，并且不会暴露指针和引用，是模板元变成的基础
	```

- ``define`` 定义的函数

	```c++
	#define CALL_WITH_MAX(a, b) f((a) > (b) ? (a) : (b))

	template<typename T>
	inline void callWithMax(const T&x, const T& y) {
		f(x > y ? x : y);
	}

	int a = 5, b = 0;
	CALL_WITH_MAX(a++, b);
	CALL_WITH_MAX(a++, b + 10); // a会有不同的累加次数
	```

### 条款03：尽可能使用const

> 显示声明可以获得编译器的帮助

- ``const`` 与迭代器

	```c++
	const std::vector<int> v1 = {1, 2, 3, 4, 5};
	// 当声明为const的时候，实际上堆空间和栈空间都是无法改变的

	auto cite = v1.begin(); // 调用 begin 的 const 重载
	std::vector<int> :: iterator ite = const_cast<std::vector<int>*>(&v1)->begin();   

	const std::vector<int>::iterator iter = vec.begin();
	// 注意 &v1 和 &(*v1.begin()) 是不同的，一个取得的栈空间的地址，一个取得的是堆空间的地址
	```

- ``const`` 与函数返回值

	- 修饰引用类型

		```c++
		const MyClass& getConstReference() {
			static MyClass obj;
			return obj;
		}

		MyClass a = getConstReference(); // 这里会调用拷贝构造函数
		MyClass b;
		b = getConstReference(); // 这里会调用赋值运算符
		// 实际发生的行为与接收方也有关系，本质其实在于对引用这一行为的理解
		// 参数传递实际上是定义接收方，函数返回实际上是发送方，接收方必须严格规定接收类型

		const int x = 1;
		const int &rx = x; // 把引用理解为一个对象的别名，常量对象必须要映射到常量引用
		int y = rx; // 当然可以用一个对象的别名声明另一个对象
		```

	- 修饰非引用类型

		```c++
		class BigData {
		public:
			void getValue() const { }
		};

		const BigData createBigData() {
			return BigData();
		}

		createBigData().getValue(); // 如果modify是非const方法，这将无法编译

		```

- ``const`` 与成员函数

  - 使用 `const` 成员函数 实现 `non-const` 成员函数

	```c++
	class MyContainer {
	public:
		// const 成员函数
		const int& getElement(size_t index) const {
			if (index >= data.size()) {
				throw std::out_of_range("Index out of range");
			}
			return data[index];
		}

		// 非 const 成员函数
		int& getElement(size_t index) {
			// 调用 const 版本的 getElement 来避免代码重复
			// 使用 const_cast 和 static_cast 来移除 const 属性
			return const_cast<int&>(static_cast<const MyContainer&>(*this).getElement(index));
		}

	private:
		std::vector<int> data;
	};
	```

- 关于 ``const`` 的一些补充

	- c++ 实现常量语义会依赖编译时的符号替换，一般不会从内存中取值，已知的特殊情况：

	```c++
	1. volatile 修饰
	2. 不是内置类型，是自定义的类型
	```

- 关于去常量性的操作

	不同的编译器可能执行不一样的标准，对于本来就是 **base** 是常量的指针执行 ``const_cast`` 操作，在 **clang** 编译器下会出问题，但是 **g++** 编译器下没有问题

### 条款04：确定对象被使用前已经被初始化

- 成员初值列的细节

	- 相较于在构造函数本体内赋值，使用成员初值列有更好的效率。这是因为在构造函数本体内赋值，首先会调用成员的默认构造函数，然后调用拷贝赋值；使用初值列直接会调用成员的拷贝构造 / 默认构造

		````c++
		class A{
		public:
			A(int x = 0) : x(x) { std::cout << "defalut ctor" << std::endl; }

			A(const A& a) { std::cout << "copy ctor" << std::endl; }

			A& operator= (const A& a) {
				std::cout << "copy assignment" << std::endl;
				return *this;
			}

		private:
			int x;
		};

		class B{
		public:
			B() : a(1) { }
		private:
			A a;
		}b; // default ctor

		class C{
		public:
			C() { a = 1; }
		private:
			A a;
		}c; // default ctor, defalut ctor, copy assignment

		````

	- __c++ 规定，成员变量的初始化动作应该发生在进入构造函数本体之前__
	- 初始化顺序和声明顺序相同

- 不同编译单元初始化次序的不确定性问题

	```c++
	class Direction {
	public:
		Direction() {
			disks = tfs.numDisks(); // ERROR 无法确定初始化次序
			disks = tfs().numDisks(); // tfs() 返回一个静态对象的引用
		}
	};
	```

## 2. 构造/析构/赋值运算

### 条款05：了解C++默认编写并调用哪些函数

- 自定义类型时，应该深入考虑 ``Big Five``
- __自定义的移动语义会导致编译器不会生成默认的拷贝语义，反之同理__
- 空类的大小会占用 `1byte`  保证每个对象有唯一的地址
- 如果空类作为基类，类的大小会被优化为0，“空白基类最优化”

### 条款06：不想使用编译器自动生成的函数，应该明确拒绝

### 条款07：为多态基类声明虚析构函数

- 应该给一个多态性质基类声明虚析构函数，如果一个类有任何虚函数，就应该有一个虚析构函数
- 一个类不作为基类或不具备多态性，不应该给他声明虚析构函数

### 条款08：别让异常逃离析构函数

- 当异常从析构函数中逃逸时， ``std::terminate()`` 会被调用
- 析构函数抛出异常，可能会中断正常的堆栈展开过程
- 堆栈展开过程

	```c++
	class Resource {
	public:
		Resource() { std::cout << "Resource acquired.\n"; }
		~Resource() { std::cout << "Resource released.\n"; }
	};

	void func() {
		Resource res; // 局部对象
		throw std::runtime_error("An error occurred"); // 抛出异常
	}

	int main() {
		try {
			func();
		} catch (const std::exception& e) {
			std::cout << "Exception caught: " << e.what() << '\n';
		}
		return 0;
	}
	```

### 条款09：不要再构造函数与析构函数中调用虚函数

- 不符合虚函数语义，子类还没有构造或者子类已经析构
- 这类调用不会下降至 `derived class`
- 在 ``derived class`` 对象的 ``base class`` 构造期间，对象的类型是 ``base class``

	```c++
	class Base {
	public:
		Base() {
			std::cout << "Base constructor. typeid(this).name() = " << typeid(*this).name() << std::endl;
		}
	};

	class Derived : public Base {
	public:
		Derived() : Base() {
			std::cout << "Derived constructor. typeid(this).name() = " << typeid(*this).name() << std::endl;
		}
	};
	Derived d;
	```

### 条款10：`operator =` 返回 `reference to *this`

- 可以处理连锁赋值情况，类似的 += 、-= 都需要返回引用

### 条款11：在`operator =`中处理自我赋值

- 处理自我赋值

	```c++
	class Widget{
		Widget& operator= (const Widget& rhs) {
			if(*this == rhs) {
				return *this;
			}
			delete pb;
			pb = new Bitmap(*rhs.pb); // 这里抛出异常会导致pb指向一块被删除的区域
			return *this;
		}
	};
	// 这里没有异常处理，不具备异常安全性
	```

- 异常安全性处理

	```c++
	class Widget{
		Widget& operator= (const Widget& rhs) {
			Bitmap* temp = pb;
			pb = new Bitmap(*rhs.pb); 
			delete temp;
			return *this;
		}
	};
	// 同时可以兼顾自我赋值处理
	```

- ``copy and swap``

	```c++
	class Widget{
		Widget& operator= (const Widget& rhs) {
			Widget temp(rhs);
			swap(temp);
			return *this;
		}
	};
	```

	```c++
	class Widget{
		Widget& operator= (Widget rhs) {
			swap(rhs);
			return *this
		}
	};
	```

### 条款12：复制对象时勿忘其每一个成分

## 3. 资源管理

### 条款13：以对象管理资源

### 条款14：在资源管理类中，小心复制行为

对于资源管理类的`copying`行为，可能需要按照以下方面来决定：

- 禁止复制。有的`RAII`对象复制并不合理，如资源管理类中管理的锁
- 对底层资源祭出“引用计数法”。有时候，我们希望保持资源，直到它的最后一个使用者被销毁，

### 条款15：在资源管理类中提供对原始资源的访问

### 条款16：成对使用 ``new`` 和 ``delete`` 时要采取相同的形式

### 条款17：以独立语句将 `new` 对象置入智能指针

```c++
processWidget(std::shared_ptr<Widget> (new Widget), priority());
/*
这样一段代码，执行三个动作 new、构造shared_ptr、执行priority，这三个动作的序列是不确定的
因此可以有这样的顺序 new、priority、shared_ptr
如果priority抛出异常会导致难以察觉的资源泄露
*/

// 解决方案是使用 make_shared， 进行原子操作
```

## 4. 设计与声明

### 条款18：让接口容易被正确使用，不易被误用

- `DDL` 问题

### 条款19：设计 ``class`` 犹如设计 ``type``

### 条款20：宁以传常引用替换传值

- 大量减少拷贝构造和析构的调用，提高效率
- 防止基类与派生类切割

	```c++
	class A{
	public:
		virtual void Display() const {
			std::cout << "A is playing" << std::endl;
		}
		virtual ~A() {}
	};

	class B : public A {
	public:
		void Display() const {
			std::cout << "B is playing" << std::endl;
		}
	};

	B b;

	void displayByValue(A a) { a.Display(); }

	displayByValue(b);

	void displayByRef(const A& a) { a.Display(); }

	displayByRef(b);

	```

### 条款21：必须返回对象时，别妄想返回其 `reference`

### 条款22：将成员变量声明为 ``private``

- 通过对象不可见来实现更精细的控制，更好的一致性

- 如果删除一个 `public` 或者 `protect` 成员变量，会有多少代码被破坏 __?__

### 条款23：宁以 ``non-member`` ``non-friend`` 替换 ``member`` 函数

- 增强封装性，实际上采用成员函数，会增加可以看到某一块数据的代码

- 提高包裹弹性，减少编译依赖

    - 比较自然的定义如下：

		```c++
		// webbrowser.h
		namespace WebBrowserStuff {
			class WebBrowser { ... };
			void clearBrowser( WebBrowser& );
			... // 核心机能，所有用户都需要的 non-member 函数
		}
		// webbrowserbookmarks.h
		namespace WebBrowserStuff {
			... // 与书签相关的函数
		} 
		// webbrowsercooikes.h
		namespace WebBrowserStuff {
			... // 与 cooikes 相关的函数
		}
		```

### 条款24：若所有参数皆需类型转换，请为此采用 ``non-member`` 函数

- 如果你需要为一个函数的所有参数（包括 ``this`` 指针所指的隐喻参数） 进行类型转换，那么这个函数必须是个 `non-member`

### 条款25：考虑写出一个不抛出异常的 ``swap`` 函数

- `pimpl` 手法

    - 降低耦合 信息隐藏 降低编译依赖 接口与实现分离

- 针对使用`pimpl` 手法的 ``swap`` 函数

	- 非模板类

		````c++
		class Widget{
		public:
			void swap(Widget& other) noexcept {
				using std::swap;
				swap(pImpl, this->pImpl);
			}
		private:
			class WidgetImpl;
			WidgetImpl* pImpl;
		};

		namespace std{
			template<>
			void swap<Widget>(Widget& w1, Widget& w2) noexcept {
				w1.swap(w2);
			}
		}
		// 不仅能够通过编译，并且和 STL 容器具有一致性
		````

	- 模板类

		``std``是一个特殊的命名空间，客户可以全特化里面的模板，但是不可以添加新的模板（不能进行模板函数的重载）

		```c++
		namespace WidgetStuff{
		template<typename T>
		class Widget{ ... };

		template<typename T>
		void swap(Widget<T>& w1, Widget<T>& w2) { ... };
		}
		```

## 5. 实现

### 条款26：尽可能延后变量定义式的出现时间

### 条款27：尽量少做转型动作

- 类型转换带来的问题

    - 多重继承的场景

		```c++
		class Base1 {
		public:
			int b1;
		};

		class Base2 {
		public:
			int b2;
		};

		class Derived : public Base1, public Base2 {
		public:
			int d;
		};

		Derived d;
		Base1* pb1 = &d;  // 指向 d 的 Base1 部分，不需要偏移
		Base2* pb2 = &d;  // 指向 d 的 Base2 部分，需要偏移
		```

    - 派生类虚函数中调用基类实现

		```c++
		class Base{
		public:
			Base(int x = 0) : x(x) {}
			virtual void Work() { ++x; }

		protected:
			int x;
		};

		class Derived : public Base {
		public: 
			virtual void Work() {
				static_cast<Base>(*this).Work(); // 这里修改的其实是一个副本
				Base::Work(); // 这里修改的才是想要的
			}
		};
		```

- ``static_cast``详解

	- `static_cast` 中的拷贝

		```c++
		class Base{
		public:
			Base(int x = 0) : x(x) { }
			virtual void Work() {
				x++;
				std::cout << "Base" << std::endl;
			}
			int& getX() {
				return x;
			}
			void Print() {
				std::cout << "x = " << x << std::endl;
			}
		protected:
			int x;
		};

		class Derived : public Base {
		public: 
			virtual void Work() {
				++getX();
				std::cout << "Derived" << std::endl;
			}
		};

		Derived d;
		Base* ptrd = &d;
		Base& refd = d;

		static_cast<Derived&>(refd).Work(); // Derived
		d.Print();	// 1

		static_cast<Derived*>(ptrd)->Work(); // Derived
		d.Print(); // 2

		static_cast<Base>(d).Work(); // Base，切片的情况会调用拷贝构造函数，构造新的对象 
		d.Print(); // 2
		```

	- `static_cast` 从左值到将亡值

		```c++
		// 实际上根据测试并不会？
		std::vector<int> v1{1, 2, 3};
		std::vector<int> v2 = static_cast<std::vector<int>&&>(v1);
		```

	- `static_cast` 初始化转换

		```c++
		std::vector<int> v = static_cast<std::vector<int>> (10);
		```

	- `static_cast` 弃值表达式

		```c++
		static_cast<void>(v.size());
		```

- ``dynamic_cast``

	- `typeid` 操作符可以用于任何表达式，并返回一个对 ``type_info`` 对象的引用。这个对象代表了表达式的类型。在多态类型中，``typeid`` 可以用来确定对象的实际派生类型。
	- `typeinfo` 对象不能直接构造或赋值，只能通过`typeid`操作符进行访问，主要方法有 `name` 和 ``operator=``
	- 每个多态类型都会有一个关联的虚表，其中包含指向该类型 `type_info` 对象的指针
	- `dynamic_cast` 一个没有虚函数的类的时候就会报错，`static_cast` 不受影响
	- 用于类之间的上下行转换，相比于 ``static_cast`` 多了类型安全转换，运行效率低

- ``const_cast``

	```c++
	int x = 1;

	const int &crefx = x; 
	const_cast<int&>(crefx) = 2;

	const int y = 1;
	const_cast<int&>(y) = 2; // 未定义行为
	const_cast<int*>(&y) = 2; // ERROR，目标类型是指针，返回右值；目标类型是引用，返回左值
	```

- ``reinterpret_cast``

### 条款28：避免返回 `handles` 指向对象内部成分

### 条款29：为“异常安全”而努力是值得的

### 条款30：透彻了解 `inlining` 里里外外

### 条款31：将文件的编译依存关系降至最低

- 依赖于声明式，而不是定义式，实现此构想的两个手段是 `Handle classes` 和 `Interface classes`

## 6. 继承与面向对象设计

### 条款32：确定你的 ``public`` 继承塑模出 ``is-a`` 关系

- 适用于 ``base classes`` 身上的每一件事情一定适用于 ``derived classes`` 身上

### 条款33：避免遮掩继承而来的名称

- ``derived classes`` 内的名称会遮掩 `base classes` 内的名称，在 `public` 继承下没有人希望如此
- 为了让遮掩的名称在见天日，可使用 ``using`` 声明式或者转交函数，转交函数可以实现只保留自己想使用的一部分

### 条款34：区分接口继承和实现继承

- 在 `pulbic` 继承的条件下，派生类总是会继承基类的接口
- 纯虚函数只继承接口
- 非纯虚函数继承接口和缺省实现
- 非虚函数继承接口和实现

```c++
class Airplane{
public: 
	virtual void fly(const Airport& destination) = 0;
	...
protected:
	void defaultFly(const Airport& destination);
};	

class ModelA : public Airplane {
public:
	virtual void fly(const Airport& destination) { defaultFly(destination); }
};
```

### 条款35：考虑 ``virtual`` 函数以外的其他选择

### 条款36：绝不重新定义继承而来的 ``non-virtual`` 函数

```c++
class Base{
public:
    virtual void mf1() = 0;
    virtual void mf1(int);
    virtual void mf2();
    void mf3();
};

class Derived : public Base {
public:
    virtual void mf1();
    void mf3();
    void mf4();
};

Derived d;
Base* pb = &d;
Derived* pd = &d;

pd->mf3(); // derived
pb->mf3(); // base，非虚函数静态绑定

d.mf1(10); // ERROR，被遮蔽了，可以通过using、定义域、转发的方式进行调用
```

### 条款37：绝不重新定义继承而来的缺省参数值

虚函数的默认参数是静态绑定的，而虚函数本身是动态绑定的，这意味着默认参数值在编译时决定，依赖于指针或引用的静态类型；虚函数的选择是在运行时进行的，根据对象的动态类型决定调用哪个版本的函数

```c++
class Base {
public:
    virtual void func(int x = 10) {
        std::cout << "Base::func with x = " << x << std::endl;
    }
};

class Derived : public Base {
public:
    void func(int x = 20) override { 
        std::cout << "Derived::func with x = " << x << std::endl;
    }
};

Derived d;
Base* pb = &d;
pb->func();   // Derived::func with x = 10
```

### 条款38：通过复合塑模出 `has-a` 或 “根据某物实现出”

### 条款39：明智而审慎地使用 ``private`` 继承

### 条款40：明智而审慎地使用多重继承

- ``c++`` 中的内存模型

	- 多重继承

		```c++
		struct A {
			int ax;
			int ay; // 这个变量不会影响C的大小，也不会影响A的大小，在内存8字节对齐的情况下
			virtual void f0() {
				std::cout << "A::f0" << std::endl;
			}
		};

		struct B {
			int bx;
			// int by; 这个变量会影响C的大小
			virtual void f1() {
				std::cout << "B::f1" << std::endl;
			}
		};

		struct C : public A, public B{
			int cx;
			// int cy; 这个变量会影响C的大小
			void f0() override {
				std::cout << "C::f0" << std::endl;
			}
			void f1() override {
				std::cout << "C::f1" << std::endl;
			}
			virtual void f2() {
				std::cout << "C::f2" << std::endl;
			}
		}c;

		/*
		*** Dumping AST Record Layout
				0 | struct C
				0 |   struct A (primary base)
				0 |     (A vtable pointer)
				8 |     int ax
			   12 |     int ay
			   16 |   struct B (base)
			   16 |     (B vtable pointer)
			   24 |     int bx
			   28 |   int cx
				  | [sizeof=32, dsize=32, align=8,
				  |  nvsize=32, nvalign=8]
		*/

		uintptr_t* vptr_A = reinterpret_cast<uintptr_t*>(&c);
		uintptr_t* vptr_B = reinterpret_cast<uintptr_t*>(reinterpret_cast<char*>(&c) + sizeof(A));
		// 这里转换char*指针，方便按照字节操作指针

		uintptr_t* vtable_A = reinterpret_cast<uintptr_t*>(*vptr_A);      
		uintptr_t* vtable_B = reinterpret_cast<uintptr_t*>(*vptr_B);

		std::cout << vtable_B - vtable_A << std::endl; // 5

		std::cout << static_cast<long>(vtable_A[-2]) << std::endl; // offset_to_top: 0
		std::cout << reinterpret_cast<std::type_info*>(vtable_A[-1])->name() << std::endl; // RTTI for C
		reinterpret_cast<void(*)()>(vtable_A[0])(); // C: f0
		reinterpret_cast<void(*)()>(vtable_A[1])(); // C: f1
		reinterpret_cast<void(*)()>(vtable_A[2])(); // C: f2

		std::cout << static_cast<long>(vtable_A[3]) << std::endl; // offset_to_top: 16
		std::cout << reinterpret_cast<std::type_info*>(vtable_A[4])->name() << std::endl; // RTTI for C

		reinterpret_cast<void(*)()>(vtable_B[0])(); // C: f1
		```

	- 棱形继承 -> 虚基类的偏移量需要在运行时候动态确定

		```c++
		struct A {
			int ax;
			virtual void f0() {
				std::cout << "A:: f0" << std::endl;
			}
			void fA() {
				std::cout << "A:: fa" << std::endl;
			}
		};

		struct B : virtual public A{
			int bx;
			virtual void f0() {
				std::cout << "B:: f0" << std::endl;        
			}
		};

		struct C : virtual public A{
			int cx;
			virtual void f0() {
				std::cout << "C:: f0" << std::endl;                
			}
		};

		struct D : public B, public C {
			int dx;
			virtual void f0() {
				std::cout << "D:: f0" << std::endl;                
			}
		}d;

		// 可以通过 clang++ -cc1 -fdump-record-layouts test.cpp 查看layouts

		/*
		*** Dumping AST Record Layout
         0 | struct D
         0 |   struct B (primary base)
         0 |     (B vtable pointer)
         8 |     int bx
        16 |   struct C (base)
        16 |     (C vtable pointer)
        24 |     int cx
        28 |   int dx
        32 |   struct A (virtual base)
        32 |     (A vtable pointer)
        40 |     int ax
           | [sizeof=48, dsize=44, align=8,
           |  nvsize=32, nvalign=8]
		*/

		// 可以通过 clang++ -cc1 -emit-llvm-only -fdump-vtable-layouts test.cpp 查看vtable布局

		/*
		Vtable for 'D' (12 entries).
		0 | vbase_offset (32)
		1 | offset_to_top (0)
		2 | D RTTI
			-- (B, 0) vtable address --
			-- (D, 0) vtable address --
		3 | void D::f0()
		4 | vbase_offset (16)
		5 | offset_to_top (-16)
		6 | D RTTI
			-- (C, 16) vtable address --
		7 | void D::f0()
			[this adjustment: -16 non-virtual] method: void C::f0()
		8 | vcall_offset (-32)
		9 | offset_to_top (-32)
		10 | D RTTI
			-- (A, 32) vtable address --
		11 | void D::f0()
			[this adjustment: 0 non-virtual, -24 vcall offset offset] method: void A::f0()	
		*/	
		```

## 7. 模板与范型编程

### 条款41：了解隐式接口和编译期多态

### 条款42：了解 ``typename`` 的双重意义

### 条款43：学习处理模版化基类内的名称

```c++
template<typename Company>
class Sendmsg {
public: 
    void Send();
};

template<typename Company>
class SendmsgWithLog : public Sendmdg<typename Company>{
public:
    void SendWithLog() {
        Send(); // ERROR! use of undeclared identifier 'Send'
		// 从面向对象迈入模板编程需要慎之又慎，编译器不确定你是否进行了偏特化导致基类没有这个函数
		// way1: this->Send();
		// way2: using
		// way3: Sendmsg<Company>::Send() 这样会导致基类的虚函数无法发挥作用，更好的解决方式是上面的方案
    }
};
```

### 条款44：将与参数无关的代码抽离 ``templates``

### 条款45：运用成员函数模板接收所有兼容类型

- 请使用请使用 `member function templates` 生成“可接受所有兼容类型” 的函数
- 如果你声明 `member templates` 用于泛化操作, 你还是需要声明正常的 ``copy ctor`` 和 `copy assignment`

### 条款46：需要类型转换时请为模板定义非成员函数

```c++
template<typename T>
class A {
public:
    A(T x = 0, T y = 0) : x(x), y(y) {}

    T getX() const { return x; }
    T getY() const { return y; }

    friend const A operator* (const A& lhs, const A& rhs) {
        return multiply(lhs, rhs);
    }
	// 通过声明友元的方式，能够使编译器声明对应类型的乘法操作，完成隐式类型转换
	// 这里需要返回一个const，防止被意外修改
private:
    T x, y;
};

template<typename T>
const A<T> multiply(const A<T>& lhs, const A<T>& rhs) {
    return A<T>(lhs.getX() * rhs.getX(), lhs.getY() * rhs.getY());
}

A<int> a(1, 1);
A<int> res = 2 * a ;
```

### 条款47：请使用 ``traits classes`` 表现类型信息

### 条款48：认识 ``template`` 元编程

## 8. 定制 ``new`` 和 ``delete``

### 条款49：了解 ``new-handler`` 的行为

### 条款50：了解 ``new`` 和 ``delete`` 的合理替换时机

- 为了收集动态分配内存之统计信息
- 为了增加分配和规划的速度
- 为了降低缺省内存管理器带来的空间额外开销
- 为了弥补缺省分配其中的非最佳齐位

	...

### 条款51：编写 ``new`` 和 ``delete`` 时需固守常规

### 条款52：写了 ``placement new`` 也要写 ``placement delete``

## 9. 杂项讨论

### 条款53：不要轻忽编译器的警告
