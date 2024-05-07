#include <iostream>
#include <new>
#include <memory>
#include <dlfcn.h>

class A {
public:
    A() {
        std::cout << "A ctor" << std::endl;
    }
};

class B {
public:
    B() {
        std::cout << "B ctor" << std::endl;
    }

    B(int) {
        std::cout << "B ctor" << std::endl;
    }
};

class C {
public:
    C() {
        std::cout << "C ctor" << std::endl;
    }
};

class Widget {
public:
    Widget() : b(1), a(), c(){

    }
private:
    A a;
    B b;
    C c;
};



int main() {

    Widget w;

}