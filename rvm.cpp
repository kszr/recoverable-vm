#include"rvm.h"
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <algorithm>
#include <stdlib.h>
#include <vector>
#include <map>
#include <dirent.h>

using namespace std;
//static trans_t tid;
// static std::vector<const char*> rvm_vector;
// static std::map<const char*, segment_t*> seg_map;
// static std::map<const char*, segment_t*>::iterator itr;
// static std::map<trans_t, std::vector<undo_log_t*> > undo_map;
// static std::map<trans_t, std::vector<redo_log_t*> > redo_map;

/* =============== Helper Functions =============== */

/**
 * Reads from the file specified by filepath and returns its contents as a string.
 */ 
char *read_from_file(char *filepath) {
    std::ifstream t;
    int length;
    t.open(filepath);                   // open input file
    t.seekg(0, std::ios::end);          // go to the end
    length = t.tellg();                 // report location (this is the length)
    t.seekg(0, std::ios::beg);          // go back to the beginning
    char *buffer = new char[length];    // allocate memory for a buffer of appropriate dimension
    t.read(buffer, length);             // read the whole file into the buffer
    t.close();                          // close file handle
    return buffer;
}

/**
 * Returns a vector of files in directory dirpath with extension ext.
 * Note that ext does not include '.' .
 */
std::vector<char*> get_file_list(std::string dirpath, std::string ext) {
    DIR *dpdf;
    struct dirent *epdf;
    std::vector<char *> vec;
    
    dpdf = opendir(dirpath.c_str());
    
    if (dpdf != NULL) {
       while ( (epdf = readdir(dpdf))) {
          std::string name(epdf->d_name);
          if(name.size()>ext.size() && 
                !(name.substr(name.size()-ext.size()-1, ext.size()+1).compare("."+ext)) )
            vec.push_back(epdf->d_name);
       }
    } 
    return vec;
}

/**
 * Gets the base name of a file from its filename. Ext must not 
 * contain the '.' character.
 */
const char *get_base_name(std::string filename, std::string ext) {
    int i = filename.find_last_of("."+ext);
    return filename.substr(0,i).c_str();
}

/**
 * Reads any .seg files that may be on disk and reconstructs segments.
 */
void restore_segs_from_disk(rvm_t *rvm) {
    std::vector<char*> seg_files = get_file_list(rvm->dirpath, "seg");
    for(auto &file : seg_files) {
        segment_t *seg = (segment_t *) malloc(sizeof(segment_t));
        char *data = read_from_file(file);
        seg->segbase = (char *) malloc(sizeof(data));
        if(data)
            sprintf(seg->segbase, data);
        seg->ismapped = 0;
        rvm->seg_map[get_base_name(file, "seg")] = seg;
    }
}

/*
  Returns the segment, if any, associated with addr segbase.
*/
segment_t *get_segment(rvm_t rvm, void *segbase) {
    std::map<const char*, segment_t*>::iterator itr;
    int found = 0;
    segment_t *segtemp = NULL;
	itr = rvm.seg_map.begin();
	while(itr != rvm.seg_map.end()) {
		found = ((long) itr-> second->segbase == (long) segbase);
		if(found) {
			segtemp = itr-> second;
            break;
		}
		itr++;
	}
    return segtemp;
}

/* =============== END Helper Functions =============== */

/**
 * Initializes an rvm_t instance with the directory name that is passed in,
 * creating the directory if necessary.
 */
rvm_t rvm_init(const char *directory) {
    printf("Init started\n");
    char buf[sizeof("mkdir ")+sizeof(directory)];
    strcpy(buf, "mkdir ");
    strcat(buf, directory);
 
    system(buf);
    
    rvm_t rvm;
    rvm.dirpath = directory;
    
    // Restores segments from disk, if any.
    restore_segs_from_disk(&rvm);
    return rvm;
}

/*
  map a segment from disk into memory. If the segment does not already exist,
  then create it and give it size size_to_create. If the segment exists but 
  is shorter than size_to_create, then extend it until it is long enough.
  It is an error to try to map the same segment twice.
*/
void *rvm_map(rvm_t rvm, const char *segname, int size_to_create) {
	//compare segname with segnames data structure
    std::map<const char*, segment_t*>::iterator itr;
	itr = rvm.seg_map.find(segname);
	if (itr != rvm.seg_map.end()) {
        itr->second->segbase = (char*) realloc ((char*)segname, size_to_create);
        itr->second->ismapped = 1;
        itr->second->ul_vector = vector<undo_log_t*>();
        itr->second->rl = NULL;
        printf("Segment already present\n");
	} else {
        segment_t *segtemp = (segment_t *) malloc(sizeof(segment_t));
		segtemp->segbase = (char *) malloc(size_to_create);
		segtemp->ismapped = 1;
        segtemp->ul_vector = vector<undo_log_t*>();
        segtemp->rl = NULL;
		rvm.seg_map[segname] = segtemp;
        
        char buf[sizeof("touch ")+sizeof(segname)+4];
        sprintf(buf,"%s%s%s","touch ",segname,".seg");
        system(buf);
        
		printf("New Segment created\n");
	}
    
    printf("Printing Key Values\n");
    
    for(itr = rvm.seg_map.begin();itr != rvm.seg_map.end(); ++itr) {
        std::cout << itr->first << "\n";
    }
    
	return (void *) rvm.seg_map[segname]->segbase;
}

/*
  unmap a segment from memory.
*/
void rvm_unmap(rvm_t rvm, void *segbase) {
	segment_t *segtemp = get_segment(rvm, segbase);
	if  (segtemp != NULL) {
		segtemp->ismapped = 0;
	}
	
    printf("Unmap Completed\n");
}

/*
  destroy a segment completely, erasing its backing store. This function should not be called on a segment that is currently mapped.
 */
void rvm_destroy(rvm_t rvm, const char *segname) {
    printf("Destroy started\n");
    if(rvm.seg_map.count(segname) == 0)
        return;
        
    segment_t *curr = rvm.seg_map[segname];
    
    if(!curr->ismapped)
        rvm.seg_map.erase(segname);
    
    free((void *) curr->segbase);
    
    char buf[sizeof("rm ")+sizeof(segname)+4];
    sprintf(buf,"%s%s%s","touch ",segname,".seg");
    system(buf);

    printf("Destroy Completed\n");
}

/*
  begin a transaction that will modify the segments listed in segbases.
  If any of the specified segments is already being modified by a transaction,
  then the call should fail and return (trans_t) -1.
  Note that trant_t needs to be able to be typecasted to an integer type.
 */
trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases){
    // Initial check to see if any of the segments is being modified.
    for(int i=0; i<numsegs; i++) {
        segment_t *segtemp = get_segment(rvm, segbases[i]);
        if  (segtemp == NULL || segtemp->busy == 1) {
            printf("Begin Transaction Failed to Complete\n");
            return -1;
        }           
    }
    
    segment_t **segtracker = new segment_t*[numsegs];
    
    // Maintain an array in memory that keeps track of which segments are being
    // prepped for a transaction. Disguise the pointer to this array as tid
    // so that it can be referenced just by passing tid to a function.
    for(int i=0; i<numsegs; i++) {
        segment_t *segtemp = get_segment(rvm, segbases[i]);
        segtracker[i] = segtemp;
        segtemp->busy = 1;
    }
            
    printf("Begin Transaction Completed\n");
    
    return (trans_t) segtracker;
}

/*
  declare that the library is about to modify a specified range of memory in the specified segment. 
  The segment must be one of the segments specified in the call to rvm_begin_trans.
  Your library needs to ensure that the old memory has been saved, in case an abort is executed.
  It is legal call rvm_about_to_modify multiple times on the same memory area.
*/
void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size){
    printf("About to Modify started with segbase %lu\n", (long) segbase);
    
    segment_t **segtracker = (segment_t **) tid;
    segment_t *seg = NULL;
    
    // Make sure that the segment being passed in can be modified - 
    // i.e., it is present in segtracker.
    int found = 0;
    for(size_t i=0; i<sizeof(segtracker)/sizeof(segment_t*); i++) {
        if(segtracker[i]->segbase == segbase) {
            found = 1;
            seg = segtracker[i];
            break;
        }
    }
    
    if(!found) {
        printf("Operation not valid!\n");
        return;
    }
    
    undo_log_t *ul = (undo_log_t *) malloc(sizeof(undo_log_t));
    ul->segbase = (char *) segbase;
    ul->offset = offset;
    
    printf("Segment Data in About to Modify %s\n", (char*)(segbase));

    ul->data = (char *) malloc(sizeof(char)*size);
    if((char *) segbase + offset)
        memcpy(ul->data, (char *) segbase + offset, size);
        
    printf("Segment Data in undo log %s\n", ul->data);
    
    seg->ul_vector.push_back(ul);
    
    printf("About to Modify Completed\n");
}

/*
commit all changes that have been made within the specified transaction.
When the call returns, then enough information should have been saved
to disk so that, even if the program crashes,
the changes will be seen by the program when it restarts.
*/
void rvm_commit_trans(trans_t tid) {
    // Get rid of undo log.
    // redo_map[tid] = std::vector<redo_log*>();
    
    segment_t **segtracker = (segment_t **) tid;
    // segment_t *seg;

    // Get rid of undo logs.
    for(size_t i=0; i<sizeof(segtracker)/sizeof(segment_t*); i++) {
        for(auto &ul : segtracker[i]->ul_vector) {
            if(ul) {
                if(ul->data) {
                    free(ul->data);
                }
                free(ul);
            }
        }
        segtracker[i]->ul_vector.clear();
    }
    
    /*
    for(int i=0; i<undo_map[tid].size(); i++) {
        undo_log *ul = undo_map[tid][i];
        segment *seg = get_segment(ul->segbase);

            
        // Create redo log.
        redo_log *rl = (redo_log *) malloc(sizeof(redo_log));
        rl->segbase = ul->segbase;
        rl->offset = ul->offset;
        
        redo_map[tid].push_back(rl);
        
        // metadata something something
        
        
        // Set busy to false;
        seg->busy = 0;
        
        // Check threshold; push to disk as necessary.
        
        // Get rid of undo log.
        
        free(ul);
    }
            
    undo_map.erase(tid);
    */
    
    delete segtracker;
    
    printf("Commit Completed\n");
}

/*
  undo all changes that have happened within the specified transaction.
 */
void rvm_abort_trans(trans_t tid){
    printf("Abort started\n");
    
    segment_t **segtracker = (segment_t **) tid;
    
    /*
    for(int i=undo_map[tid].size()-1; i>=0; i--) {
        undo_log *ul = undo_map[tid][i];
        segment *seg = get_segment(ul->segbase);
        
        // Copy from undo log into segment.
        if(ul->data)
            memcpy(seg->segbase+ul->offset,ul->data,sizeof(ul->data)/sizeof(ul->data[0]));
        
        // Set busy to false
        seg->busy = 0;
        
        undo_map[tid].pop_back();
        free(ul);
    }   
            
    // Get rid of undo log.
    undo_map.erase(tid);
    */
    
    delete segtracker;
    
    printf("Abort Completed\n");
}

/*
 play through any committed or aborted items in the log file(s) and shrink the log file(s) as much as possible.
*/
void rvm_truncate_log(rvm_t rvm){
	printf("Truncating\n");

}

