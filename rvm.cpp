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
static char *read_from_file(std::string filepath) {
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
static std::vector<std::string> get_file_list(std::string dirpath, std::string ext) {
    DIR *dpdf;
    struct dirent *epdf;
    std::vector<std::string> vec;
    
    dpdf = opendir(dirpath.c_str());
    
    if (dpdf != NULL) {
       while ( (epdf = readdir(dpdf))) {
          std::string name(epdf->d_name);
          if(name.size()>ext.size() && 
                !(name.substr(name.size()-ext.size()-1, ext.size()+1).compare("."+ext)) )
            vec.push_back(name);
       }
    } 
    return vec;
}

/**
 * Returns all log files corresponding to a given segment.
 */
static std::vector<std::string> get_log_files(std::string segname) {
    std::vector<std::pair<int, std::string> > vec;
    for(auto &f : get_file_list("./", "log")) {
        if(f.find(segname) == 0) {
            int i = std::stoi(f.substr(segname.length(), f.length() - segname.length() - 4));
            vec.push_back(std::pair<int, std::string>(i, f));
        }
    }
    
    // Make sure log files are sorted by increasing number. Have to do this to avoid
    // sorting numbers alphabetically.
    std::sort(vec.begin(), vec.end(), 
        [](std::pair<int, std::string> const &a, std::pair<int, std::string> const &b) {
        return a.first < b.first; 
    });
    
    std::vector<std::string> new_vec = std::vector<std::string>();
    for(auto &p : vec) {
        new_vec.push_back(p.second);
    }
    
    return new_vec;
}

/**
 * Gets the base name of a file from its filename. Ext must not 
 * contain the '.' character.
 */
static std::string get_base_name(std::string filename, std::string ext) {
    if(filename.length() < 4)
        return NULL;
    size_t i = filename.find("."+ext,filename.length()-4);
    return i != std::string::npos ? filename.substr(0,i).c_str() : NULL;
}

/**
 * Reads any .seg files that may be on disk and reconstructs segments.
 */
static void restore_segs_from_disk(rvm_s *rvm) {
    printf("Restoring segments from disk\n");
    
    std::vector<std::string> seg_files = get_file_list(rvm->dirpath, "seg");
    for(auto &file : seg_files) {
        std::string segname = get_base_name(file, "seg");
        segment_t *seg = new segment_t;
        char *data = read_from_file(rvm->dirpath + file);
        seg->segbase = (char *) malloc(sizeof(data)/sizeof(char));
        if(data)
            sprintf(seg->segbase, data);
        seg->ismapped = 0;
        seg->dirpath = rvm->dirpath;
        seg->segname = segname;
        seg->ul_vector = std::vector<undo_log_t*>();
        seg->rl_vector = std::vector<redo_log_t*>();
        seg->size = sizeof(data)/sizeof(char);
        rvm->seg_map[seg->segname.c_str()] = seg;
    }
}

/*
  Returns the segment, if any, associated with addr segbase.
*/
static segment_t *get_segment(rvm_t rvm, void *segbase) {
    std::map<std::string, segment_t*>::iterator itr;
    int found = 0;
    segment_t *segtemp = NULL;
    itr = rvm->seg_map.begin();
    while(itr != rvm->seg_map.end()) {
        found = ((long) itr-> second->segbase == (long) segbase);
        if(found) {
            segtemp = itr-> second;
            break;
        }
        itr++;
    }
    return segtemp;
}

/**
 * Returns a unique number for a file...
 * E.g., segname0.seg or segname1023.log.
 */
static int get_unique_file_num(std::string dirpath, std::string segname, std::string ext) {
    if(dirpath.length() < 1)
        dirpath = "./";
        
    if(dirpath[dirpath.length()-1] != '/')
        dirpath = dirpath + "/";
    
    std::vector<std::string> file_list = get_file_list(dirpath,ext);
    std::vector<std::string> counter;
    
    for(auto &f : file_list)
        if(f.find(segname + "_") != std::string::npos)
            counter.push_back(f);
            
    return counter.size();
}

/**
 * Writes data to a file, given segname and ext (e.g., "seg" or "log"). Note
 * ext does not contain the '.' character.
 */
// static void write_to_file(char *data, std::string segname, std::string ext) {
//     int num = get_unique_file_num(segname, ext);
//     std::ofstream outputFile(segname + std::to_string(num) + "." + ext);
//     outputFile << data;
//     outputFile.close();
// }

/* =============== END Helper Functions =============== */

/**
 * Initializes an rvm_t instance with the directory name that is passed in,
 * creating the directory if necessary.
 */
rvm_t rvm_init(const char *directory) {
    printf("Init started\n");

    rvm_s *rvm = new rvm_s;
    rvm->seg_map = std::map<std::string, segment_t*>();
    rvm->dirpath = directory;

    if(sizeof(directory)/sizeof(char) < 1)
        rvm->dirpath = "./";

    if(rvm->dirpath[rvm->dirpath.length()-1] != '/')
        rvm->dirpath = rvm->dirpath + "/";

    std::string buf = "mkdir " + rvm->dirpath;
    system(buf.c_str());

    // Restores segments from disk, if any.
    restore_segs_from_disk(rvm);
    
    return (rvm_t) rvm;
}

/*
  map a segment from disk into memory. If the segment does not already exist,
  then create it and give it size size_to_create. If the segment exists but 
  is shorter than size_to_create, then extend it until it is long enough.
  It is an error to try to map the same segment twice.
*/
void *rvm_map(rvm_t rvm, const char *segname, int size_to_create) {
    //compare segname with segnames data structure
    printf("Begin map\n");

    std::map<std::string, segment_t*>::iterator itr;
    itr = rvm->seg_map.find(std::string(segname));
    if (itr != rvm->seg_map.end()) {
        itr->second->segbase = (char*) realloc ((char*) itr->second->segbase, size_to_create);
        itr->second->size = size_to_create;
        itr->second->ismapped = 1;
        itr->second->ul_vector = std::vector<undo_log_t*>();
        itr->second->rl_vector = std::vector<redo_log_t*>();

        printf("Segment already present\n");
    } else {
        segment_t *segtemp = new segment_t;
        segtemp->segbase = (char *) malloc(size_to_create);
        segtemp->ismapped = 1;
        segtemp->segname = segname;
        segtemp->size = size_to_create;
        segtemp->dirpath = rvm->dirpath;
        segtemp->ul_vector = std::vector<undo_log_t*>();
        segtemp->rl_vector = std::vector<redo_log_t*>();
        rvm->seg_map[segtemp->segname] = segtemp;

        std::string buf = "touch " + rvm->dirpath + std::string(segname) + ".seg";
        system(buf.c_str());

        printf("New Segment created\n");
    }

    printf("Printing Segnames\n");

    for(itr = rvm->seg_map.begin();itr != rvm->seg_map.end(); ++itr) {
        std::cout << "[" << itr->first << "]\n";
    }

    return (void *) rvm->seg_map[segname]->segbase;
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

    if(rvm->seg_map.count(segname) == 0)
        return;

    segment_t *curr = rvm->seg_map[segname];

    if(!curr->ismapped)
        rvm->seg_map.erase(segname);
    else return;

    if(curr->segbase)
        free((void *) curr->segbase);

    delete curr;

    std::string buf = "rm " + std::string(segname) + ".seg";
    system(buf.c_str());

    printf("Destroy Completed\n");
}

/*
  begin a transaction that will modify the segments listed in segbases.
  If any of the specified segments is already being modified by a transaction,
  then the call should fail and return (trans_t) -1.
  Note that trant_t needs to be able to be typecasted to an integer type.
 */
trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases) {    
    std::map<std::string, segment_t*>::iterator itr;

    printf("Printing segnames in begin_transaction\n");
    std::cout << "Seg map size = " << rvm->seg_map.size() << std::endl;
    for(itr = rvm->seg_map.begin(); itr != rvm->seg_map.end(); ++itr) {
        std::cout << itr->first << ".....BOB!" << "\n";
    }

    // Initial check to see if any of the segments is being modified.
    for(int i=0; i<numsegs; i++) {
        segment_t *segtemp = get_segment(rvm, segbases[i]);
        if  (segtemp == NULL || segtemp->busy == 1) {
            printf("Begin Transaction Failed to Complete\n");
            return -1;
        }           
    }
    
    std::vector<segment_t*> *segtracker = new std::vector<segment_t*>(numsegs);

    // Maintain an array in memory that keeps track of which segments are being
    // prepped for a transaction. Disguise the pointer to this array as tid
    // so that it can be referenced just by passing tid to a function.
    for(int i=0; i<numsegs; i++) {
        segment_t *segtemp = get_segment(rvm, segbases[i]);
        (*segtracker)[i] = segtemp;
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
void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size) {
    printf("About to Modify started with segbase %lu\n", (long) segbase);

    std::vector<segment_t*> *segtracker = (std::vector<segment_t*> *) tid;
    // segment_t **segtracker = (segment_t **) tid;
    segment_t *seg = NULL;

    // Make sure that the segment being passed in can be modified - 
    // i.e., it is present in segtracker.
    int found = 0;
    for(size_t i=0; i<(*segtracker).size(); i++) {
        if( (*segtracker)[i]->segbase == segbase) {
            found = 1;
            seg = (*segtracker)[i];
            break;
        }
    }
    
    if(!found) {
        printf("Operation not valid!\n");
        return;
    }
    
    undo_log_t *ul = new undo_log_t;
    ul->segbase = (char *) segbase;
    ul->offset = offset;

    printf("Segment Data in About to Modify %s\n", (char*)(segbase));

    ul->data = (char *) malloc(size);

    if(segbase && offset+size < seg->size)
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
    printf("Begin commit\n");
    
    std::vector<segment_t*> *segtracker = (std::vector<segment_t*> *) tid;
    // segment_t **segtracker = (segment_t **) tid;
    
    // Infer redo log contents from undo log perhaps?
    // The first undo log contains the original contents of the file, so skip over it.
    // Need to encode the current version of the file as a redo log containing its diff with
    // gilast undo log???
    for(size_t i=0; i<(*segtracker).size(); i++) {
        segment_t *seg = (*segtracker)[i];
        
        for(size_t j=0; j<seg->ul_vector.size(); j++) {
            undo_log_t *ul = seg->ul_vector[j];
            
            // redo_log_t *rl = new redo_log_t;
            redo_log_t rl;
            
            // Redo logs are the same as undo logs, except that their data are off by one -
            // i.e., the first undo log will store the data of the original segment,
            // but the first redo log will store what the second undo log stores at
            // the same offset as that specified by the first undo log.
            rl.offset = ul->offset;
            rl.segbase = ul->segbase;
            if(j<seg->ul_vector.size()-1) {
                rl.data = seg->ul_vector[j]->data;
            } else {
                char *data = (char *) malloc(sizeof(ul->data));
                memcpy(data, seg->segbase+rl.offset, sizeof(data)/sizeof(char));
            }
            
            // segtracker[i]->rl_vector.push_back(rl);
            
            // Write the redo log to a file.
            int num = get_unique_file_num(seg->dirpath, seg->segname, "log");
            std::string filename = seg->dirpath + seg->segname + "_" + std::to_string(num) + ".log";
            
            std::ofstream outputFile(filename);
            outputFile << rl.offset << std::endl;
            outputFile << sizeof(rl.data)/sizeof(char) << std::endl;
            outputFile << rl.data;
            outputFile.close();
            // TODO: Implement
        }
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
    
    // Set segments to not busy.
    for(size_t i=0; i<sizeof(segtracker)/sizeof(segment_t*); i++) {
        (*segtracker)[i]->busy = 0;
    }
    
    // Get rid of undo logs.
    for(size_t i=0; i<sizeof(segtracker)/sizeof(segment_t*); i++) {
        for(size_t j=0; j<(*segtracker)[i]->ul_vector.size(); j++) {
            undo_log_t *ul = (*segtracker)[i]->ul_vector[j];
            if(ul) {
                // Don't free all ul->data, since redo log is referencing it. Except for j==0.
                if(ul->data)
                    free(ul->data);
                delete ul;
            }
        }
        (*segtracker)[i]->ul_vector.clear();
    }
    
    delete segtracker;
    
    printf("Commit Completed\n");
}

/*
  undo all changes that have happened within the specified transaction.
 */
void rvm_abort_trans(trans_t tid) {
    printf("Abort started\n");
    return;
    // segment_t **segtracker = (segment_t **) tid;
    
    std::vector<segment_t*> *segtracker = (std::vector<segment_t*> *) tid;
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
void rvm_truncate_log(rvm_t rvm) {
	printf("Truncating\n");
    std::map<std::string, segment_t*>::iterator itr;
    for(itr = rvm->seg_map.begin();itr != rvm->seg_map.end(); ++itr) {
        std::string segname = itr->first;
        // TODO: Load segment from file.
        
        std::vector<std::string> log_list = get_log_files(segname);
        for(auto &lg : log_list) {
            // TODO: Apply log files to the segment.
        }
        
        // TODO: Write updated segment back into file.
        
        // TODO: Delete log files.
      }
}
