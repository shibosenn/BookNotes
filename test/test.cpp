#include <iostream>
#include <new>
#include <memory>
#include <dlfcn.h>


int main() {
    std::cout << "pre open" << std::endl;

    void *dl = dlopen("./libfoo.so", RTLD_NOW);

    if(dl == nullptr) {
        std::cout << "can not open" << std::endl;
    }

    std:: cout << "after open" << std::endl;

    dlclose(dl);
}