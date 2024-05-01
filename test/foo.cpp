#include <iostream>

class A{
public:
    A() {
        std::cout << "ctor" << std::endl;
    }
    ~A() {
        std::cout << "dtor" << std::endl;
    }
};

A *a = new A;