/* truncate the log; manually inspect to see that the log has shrunk
 * to nothing */

#include "../rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) 
{
     rvm_t rvm;
     rvm = rvm_init("rvm_segments");
     printf("\nTesting directory creation:\n");
     system("ls -l rvm_segments");

     return 0;
}
