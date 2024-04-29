#include <iostream>
#include <new>
#include <memory>


int main() {

    auto deleter = [](int *p){
        delete []p;
    };
    std::shared_ptr<int> sp{new int[3]{1, 2, 3}, deleter};
    auto dp = std::get_deleter<decltype(deleter)>(sp);

    // *dp = [](int *p) {
    //     std::cout << "chage successful" << std::endl;
    //     delete []p;
    // };


    // int *p = new int [10];
    // deleter(p);

    return 0;

}