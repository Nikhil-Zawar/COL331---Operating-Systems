#include "types.h"
#include "stat.h"
#include "user.h"

int fib(int n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    if (n == 2) return 1;
    return fib(n - 1) + fib(n - 2);
}
void more() {
    printf(1, "Final call\n");
}

void sv() {
    printf(1, "I am Shivam\n");
    more();
}

void sighuphandler(){
	printf(1, "I am inside the handler\n");
    sv();
}

int main() {
    signal(sighuphandler);
    while (1) {
        printf(1, "This is normal code running\n");
        fib(35);
    }
}