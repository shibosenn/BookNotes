#include <sys/types.h>

#include <fcntl.h>

#include <pthread.h>

int foo() {
    const int x = 1;

    int *px = &x;
    *px = 2;
    
    return x;

}

int main() {
    
}