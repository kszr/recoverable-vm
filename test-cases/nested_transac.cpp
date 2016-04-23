/*
gives an error when specified segment is already being modified by another transaction. The call fails and returns (trans_t) -1. 
*/
#include "../rvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define TEST_STRING1 "hello, world"
#define TEST_STRING2 "bleg!"
#define TEST_STRING3 "meow!"
#define OFFSET2 1000

int main(int argc, char **argv)
{
     rvm_t rvm;
     char *seg;
     char *segs[1];
     trans_t trans;
  trans_t trans1;

     rvm = rvm_init("rvm_segments");
     rvm_destroy(rvm, "testseg");
     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);
     seg = segs[0];

     /* write some data and commit it */
     trans = rvm_begin_trans(rvm, 1, (void**) segs);
     rvm_about_to_modify(trans, seg, 0, 1000);
     sprintf(seg, TEST_STRING1);

     //rvm_commit_trans(trans);

     trans1 = rvm_begin_trans(rvm, 1, (void**) segs);
    //  rvm_about_to_modify(trans1, seg, 100, 1000);
    //  sprintf(seg, TEST_STRING2);

     rvm_commit_trans(trans);

	// rvm_commit_trans(trans1);
     //rvm_abort_trans(trans);

     if((long) trans1 != -1) {
         printf("ERROR: Nested trans did not return error.\n");
         exit(1);
     }

     rvm_unmap(rvm, seg);
     printf("OK\n");
     exit(0);
}

