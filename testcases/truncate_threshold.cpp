/* truncate_threshold.cpp
 Tests whether logs are truncated after threshold is reached */

#include "../rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <dirent.h>

#define TEST_STRING1 "hello, world"
#define TEST_STRING2 "bleg!"
#define OFFSET2 1000


int main(int argc, char **argv)
{
     rvm_t rvm;
     char *seg;
     char *segs[1];
     trans_t trans;
     
     rvm = rvm_init("rvm_segments");
     
     rvm_destroy(rvm, "testseg");
     
     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);
     seg = segs[0];

     /* write some data and commit it */
     for(int i=0; i<TRUNCATE_THRESHOLD; i++) {
        trans = rvm_begin_trans(rvm, 1, (void**) segs);
        rvm_about_to_modify(trans, seg, 0, 100);
        sprintf(seg, TEST_STRING1);
        rvm_commit_trans(trans);
     }
     
     /* Check to see that no log files exist for this segment. */
     
     DIR *dpdf;
     struct dirent *epdf;
     std::vector<std::string> vec;
     std::string dirpath = "rvm_segments";
     std::string ext = "log";
     
     dpdf = opendir(dirpath.c_str());
     
     if (dpdf != NULL) {
         while ( (epdf = readdir(dpdf))) {
             std::string name(epdf->d_name);
             if(name.size()>ext.size() && 
                    !(name.substr(name.size()-ext.size()-1, ext.size()+1).compare("."+ext)) &&
                    name.find("testseg") == 0 &&
                    name[7] == '_')
                 vec.push_back(name);
         }
     }
     
     /* test that the data was restored */
     if(vec.size() > 0) {
	  printf("ERROR: Logs were not truncated after threshold (%d) was reached.\n", TRUNCATE_THRESHOLD);
	  exit(2);
     }

     rvm_unmap(rvm, seg);
     printf("OK\n");
     exit(0);
}

