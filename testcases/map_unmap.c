/* map_unmap.c:
Can't map the same segment twice without unmapping between calls to rvm_map().
 */

#include "../rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define TEST_STRING "hello, world"
#define OFFSET2 1000

int main(int argc, char **argv)
{
     char* segs[1];
     rvm_t rvm;
     
     rvm = rvm_init("rvm_segments");

     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);
     
     void *bad_seg = rvm_map(rvm, "testseg", 10000);
     
     rvm_unmap(rvm, segs[0]);
     
     void *good_seg = rvm_map(rvm, "testseg", 10000);
     
     if(bad_seg != NULL) {
	  printf("ERROR: Did not catch double map error\n");
	  exit(2);
     }
     if(good_seg == NULL) {
	  printf("ERROR: Failed to map after unmap\n");
	  exit(2);
     }

     printf("OK\n");
     exit(0);
}
