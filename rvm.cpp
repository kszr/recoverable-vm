#include"rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <vector>
#include <map>

static rvm_t g_id = 0;
static std::vector<rvm> rvm_vector;
static std::map<const char*, segment> seg_map;
static std::map<const char*, segment>::iterator itr;

/*
  Initialize the library with the specified directory as backing store.
*/
rvm_t rvm_init(const char *directory){
    rvm rvm_obj;
    char buf[sizeof("mkdir ")+sizeof(directory)];
    strcpy(buf, "mkdir ");
    strcat(buf, directory);
 
    int i = system(buf);
    
    // TODO: Find out how to tell if mkdir failed...
    if(i<0) {
        fprintf(stderr, "Error: Directory already exists!\n");
        exit(1); // Throw error
    }
    
    rvm_obj.dir_path = buf; // <== TODO: full path
    rvm_obj.id = g_id++;
    rvm_vector.push_back(rvm_obj);
    return rvm_obj.id;
}

/*
  map a segment from disk into memory. If the segment does not already exist,
  then create it and give it size size_to_create. If the segment exists but 
  is shorter than size_to_create, then extend it until it is long enough.
  It is an error to try to map the same segment twice.
*/
void *rvm_map(rvm_t rvm, const char *segname, int size_to_create){
	segment sgt;
	//compare segname with segnames data structure
	itr = seg_map.find(segname);
	if (itr != seg_map.end() && sgt.ismapped == 1) {
			sgt.segaddr = (char*) realloc ((char*)segname, size_to_create);
			seg_map.erase (itr);
			seg_map [segname] = sgt;
	} else {
		sgt.segaddr = (char *) malloc(size_to_create);
		sgt.ismapped = 1;
		seg_map [segname] = sgt;
		}
	return (void *) sgt.segaddr;
}

/*
  unmap a segment from memory.
*/
void rvm_unmap(rvm_t rvm, void *segbase){
	int found = 0;
	itr = seg_map.begin();
	while(itr != seg_map.end()) {
		found = ((long) itr-> second.segaddr == (long) segbase);
		if(found) {
			itr-> second.ismapped = 0;
			break;
		}
		itr++;
	}
	
}

/*
  destroy a segment completely, erasing its backing store. This function should not be called on a segment that is currently mapped.
 */
void rvm_destroy(rvm_t rvm, const char *segname){
    if(seg_map.count(segname) == 0)
        return;
        
    segment curr = seg_map.at(segname);
    
    if(curr.kill)
        seg_map.erase(segname);
    
    free((void *) curr.segaddr);
}

/*
  begin a transaction that will modify the segments listed in segbases. If any of the specified segments is already being modified by a transaction, then the call should fail and return (trans_t) -1. Note that trant_t needs to be able to be typecasted to an integer type.
 */
trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases){
    for(int i=0; i<numsegs; i++) {
        // Check to see if any of the segments are being modified by a a transaction.   
    }

    return 0;
}

/*
  declare that the library is about to modify a specified range of memory in the specified segment. 
  The segment must be one of the segments specified in the call to rvm_begin_trans.
  Your library needs to ensure that the old memory has been saved, in case an abort is executed.
  It is legal call rvm_about_to_modify multiple times on the same memory area.
*/
void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size){


}

/*
commit all changes that have been made within the specified transaction. When the call returns, then enough information should have been saved to disk so that, even if the program crashes, the changes will be seen by the program when it restarts.
*/
void rvm_commit_trans(trans_t tid){

}

/*
  undo all changes that have happened within the specified transaction.
 */
void rvm_abort_trans(trans_t tid){


}

/*
 play through any committed or aborted items in the log file(s) and shrink the log file(s) as much as possible.
*/
void rvm_truncate_log(rvm_t rvm){


}

