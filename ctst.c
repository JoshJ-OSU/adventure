#include "stdio.h"

char * myfunc(void);

int main() {
     
     printf("%s\n", myfunc());
     
     return 0;
}

char * myfunc(void) {
     return ("Hello Friend");
     
}
