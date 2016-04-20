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

/* =============== Helper Functions =============== */

/**
 * Reads from the file specified by filepath and returns its contents as a string.
 */ 
static char *read_from_file(std::string filepath, size_t *segsize) {
    std::ifstream t;
    int length;
    t.open(filepath);                   // open input file
    t.seekg(0, std::ios::end);          // go to the end
    length = t.tellg();                 // report location (this is the length)
    t.seekg(0, std::ios::beg);          // go back to the beginning
    char *buffer = new char[length];    // allocate memory for a buffer of appropriate dimension
    t.read(buffer, length);             // read the whole file into the buffer
    t.close();                          // close file handle
    *segsize = length - 1;
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
static std::vector<std::string> get_log_files(std::string dirpath, std::string segname) {
    std::vector<std::pair<int, std::string> > vec;
    for(auto &f : get_file_list(dirpath, "log")) {
        if(f.find(segname) == 0 && f[segname.length()] == '_') {
            int i = std::stoi(f.substr(segname.length()+1, f.length() - segname.length() - 5));
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
    printf("INFO: Restoring segments from disk\n");
    
    std::vector<std::string> seg_files = get_file_list(rvm->dirpath, "seg");
    for(auto &file : seg_files) {
        std::string segname = get_base_name(file, "seg");
        segment_t *seg = new segment_t;
        seg->ismapped = 0;
        seg->dirpath = rvm->dirpath;
        seg->segname = segname;
        seg->ul_vector = std::vector<undo_log_t*>();
        seg->rl_vector = std::vector<redo_log_t*>();
        rvm->seg_map[seg->segname.c_str()] = seg; 
    }
    
    // Truncate logs.
    rvm_truncate_log(rvm);
    
    for(auto &file : seg_files) {
        std::string segname = get_base_name(file, "seg");
        segment_t *seg = rvm->seg_map[segname];
        seg->segbase = read_from_file(rvm->dirpath + file, &seg->size);
    }
    
    printf("INFO: Done restoring segments from disk\n");
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
        found = ((long) itr->second->segbase == (long) segbase);
        if(found) {
            segtemp = itr->second;
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
 * Deletes all log files associated with a given segname in a given directory.
 */
static void delete_log_files(std::string dirpath, std::string segname) {
    std::vector<std::string> log_list = get_file_list(dirpath, "log");
    for(auto &f : log_list) {
        int i;
        if( (i = f.find(segname)) != std::string::npos && f.at(i+segname.length()) == '_') {
            std::string buf = "rm " + dirpath + f;
            system(buf.c_str());
        }
    }
}

/**
 * Creates a redo log for the given segment.
 */
static void make_redo_log(segment_t *seg) {
    // Create a redo log containing the last changes, if any, before they potentially
    // get overwritten.
    if(seg->ul_vector.size() > 0) {
        undo_log_t *last_ul = seg->ul_vector[seg->ul_vector.size()-1];
        
        if(last_ul->redone)
            return;
        
        last_ul->redone = 0;
        
        redo_log_t *rl = new redo_log_t;
        rl->offset = last_ul->offset;
        rl->size = last_ul->size;
        rl->data = new char[rl->size+1];

        char *ptr1 = rl->data;
        char *ptr2 = seg->segbase + rl->offset; 
        for(int i=0; i<rl->size; i++)
            *(ptr1++) = *(ptr2++);
        
        rl->data[rl->size] = '\0';
        
        seg->rl_vector.push_back(rl);
    }
}

/* =============== END Helper Functions =============== */

/**
 * Initializes an rvm_t instance with the directory name that is passed in,
 * creating the directory if necessary.
 */
rvm_t rvm_init(const char *directory) {
    // TODO: Figure out a way to detect multiple calls to rvm_init() from the same process.
    
    printf("INFO: Init started\n");

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
    printf("INFO: Begin map\n");

    std::map<std::string, segment_t*>::iterator itr;
    itr = rvm->seg_map.find(std::string(segname));
    if (itr != rvm->seg_map.end()) {
        itr->second->segbase = (char*) realloc ((char*) itr->second->segbase, size_to_create*sizeof(char)+1);
        itr->second->size = size_to_create;
        itr->second->ismapped = 1;
        itr->second->ul_vector = std::vector<undo_log_t*>();
        itr->second->rl_vector = std::vector<redo_log_t*>();

        printf("INFO: Segment already present\n");
    } else {
        segment_t *segtemp = new segment_t;
        segtemp->segbase = (char *) malloc(size_to_create*sizeof(char)+1);
        segtemp->ismapped = 1;
        segtemp->segname = segname;
        segtemp->size = size_to_create;
        segtemp->dirpath = rvm->dirpath;
        segtemp->ul_vector = std::vector<undo_log_t*>();
        segtemp->rl_vector = std::vector<redo_log_t*>();
        rvm->seg_map[segtemp->segname] = segtemp;

        // Initializes the segment to null at each byte.
        std::string filename = rvm->dirpath + std::string(segname) + ".seg";
        std::ofstream outputFile(filename);
        char *ptr = segtemp->segbase;
        for(int i=0; i<segtemp->size; i++) {
            outputFile << '\0';
            segtemp->segbase[i] = '\0';
        }
        outputFile << '\0';
        segtemp->segbase[size_to_create] = '\0';
        outputFile.close();
        
        printf("INFO: New Segment created\n");
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

    printf("INFO: Unmap Completed\n");
}

/*
  destroy a segment completely, erasing its backing store. This function should not be called on a segment that is currently mapped.
 */
void rvm_destroy(rvm_t rvm, const char *segname) {
    if(rvm->seg_map.count(segname) == 0)
        return;

    segment_t *curr = rvm->seg_map[segname];

    if(!curr->ismapped)
        rvm->seg_map.erase(segname);
    else return;

    if(curr->segbase)
        free((void *) curr->segbase);

    delete curr;
    
    // Remove all seg files.
    std::vector<std::string> seg_list = get_file_list(rvm->dirpath, "seg");
    for(auto &f : seg_list) {
        int i;
        if( (i = f.find(segname)) != std::string::npos && f.at(i+sizeof(segname)/sizeof(char)-1) == '.') {
            std::string buf = "rm " + rvm->dirpath + f;
            system(buf.c_str());
        }
    }
    
    delete_log_files(rvm->dirpath, segname);
}

/*
  begin a transaction that will modify the segments listed in segbases.
  If any of the specified segments is already being modified by a transaction,
  then the call should fail and return (trans_t) -1.
  Note that trant_t needs to be able to be typecasted to an integer type.
 */
trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases) {    
    std::map<std::string, segment_t*>::iterator itr;

    // Initial check to see if any of the segments is being modified.
    for(int i=0; i<numsegs; i++) {
        segment_t *segtemp = get_segment(rvm, segbases[i]);
        if  (segtemp == NULL || segtemp->busy == 1) {
            printf("ERROR: Begin Transaction Failed to Complete\n");
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
        
    printf("INFO: Begin Transaction Completed\n");

    return (trans_t) segtracker;
}

/*
  declare that the library is about to modify a specified range of memory in the specified segment. 
  The segment must be one of the segments specified in the call to rvm_begin_trans.
  Your library needs to ensure that the old memory has been saved, in case an abort is executed.
  It is legal call rvm_about_to_modify multiple times on the same memory area.
*/
void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size) {
    std::vector<segment_t*> *segtracker = (std::vector<segment_t*> *) tid;
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
        printf("ERROR: Segment not found!\n");
        return;
    }
    
    if(offset+size >= seg->size) {
        printf("ERROR: Transaction would exceed segment bounds!\n");
        return;
    }
    
    // Make a redo log here in case the user decides to make multiple
    // changes within a transaction before committing it.
    make_redo_log(seg);
            
    undo_log_t *ul = new undo_log_t;
    ul->segbase = (char *) segbase;
    ul->offset = offset;
    ul->redone = 0;
    ul->data = new char[size+1];
    ul->size = size;
    
    // Populate ul->data the old-fashioned way.
    char *ptr1 = ul->data;
    char *ptr2 = (char *) segbase + offset;
    for(int i=0; i<size; i++)
        *(ptr1++) = *(ptr2++);
    ul->data[size] = '\0';

    seg->ul_vector.push_back(ul);

    printf("INFO: About to Modify Completed\n");
}

/*
commit all changes that have been made within the specified transaction.
When the call returns, then enough information should have been saved
to disk so that, even if the program crashes,
the changes will be seen by the program when it restarts.
*/
void rvm_commit_trans(trans_t tid) {
    printf("INFO: Begin commit\n");
    
    std::vector<segment_t*> *segtracker = (std::vector<segment_t*> *) tid;

    // Write redo logs to disk.
    for(size_t i=0; i<(*segtracker).size(); i++) {
        segment_t *seg = (*segtracker)[i];
        
        // Create a redo log for this transaction.
        make_redo_log(seg);
        
        // Write redo logs to disk.
        for(int j=seg->rl_vector.size()-1; j>=0; j--) {
            redo_log_t *rl = seg->rl_vector[j];
            
            // Write the redo log to a file.
            int num = get_unique_file_num(seg->dirpath, seg->segname, "log");
            std::string filename = seg->dirpath + seg->segname + "_" + std::to_string(num) + ".log";
            
            std::ofstream outputFile(filename);
            outputFile << rl->offset << std::endl;
            outputFile << rl->size << std::endl;
            
            // Writing data character by character.
            for(int k=0; k<rl->size; k++)
                outputFile << rl->data[k];
             
            outputFile.close();
            std::cout << "INFO: Wrote " << filename << std::endl;
        }
    }
    
    // Set segments to not busy.
    for(size_t i=0; i<(*segtracker).size(); i++) {
        (*segtracker)[i]->busy = 0;
    }
    
    // Get rid of undo logs.
    for(size_t i=0; i<(*segtracker).size(); i++) {
        for(size_t j=0; j<(*segtracker)[i]->ul_vector.size(); j++) {
            undo_log_t *ul = (*segtracker)[i]->ul_vector[j];
            if(ul) {
                if(ul->data)
                    delete ul->data;
                delete ul;
            }
        }
        (*segtracker)[i]->ul_vector.clear();
    }
    
    // Get rid of redo logs.
    for(size_t i=0; i<(*segtracker).size(); i++) {
        for(size_t j=0; j<(*segtracker)[i]->rl_vector.size(); j++) {
            redo_log_t *rl = (*segtracker)[i]->rl_vector[j];
            if(rl) {
                if(rl->data)
                    delete rl->data;
                delete rl;
            }
        }
        (*segtracker)[i]->rl_vector.clear();
    }
    
    segtracker->clear();
    delete segtracker;
    
    printf("INFO: Commit complete\n");
}

/*
  undo all changes that have happened within the specified transaction.
 */
void rvm_abort_trans(trans_t tid) {
    printf("INFO: Abort started\n");
    
    std::vector<segment_t*> *segtracker = (std::vector<segment_t*> *) tid;

    for(auto &seg : *segtracker) {
        // Copy data from undo logs in the reverse of the order in which they were created.
        for(int i=seg->ul_vector.size()-1; i>=0; i--) {
            undo_log_t *ul = seg->ul_vector[i];
            char *ptr1 = (char *) seg->segbase + ul->offset;
            char *ptr2 = ul->data;
            
            for(int j=0; j<ul->size; j++)
                *(ptr1++) = *(ptr2++); 
        }
        seg->busy = 0;
    }
    
    // Get rid of undo logs.
    for(size_t i=0; i<(*segtracker).size(); i++) {
        for(size_t j=0; j<(*segtracker)[i]->ul_vector.size(); j++) {
            undo_log_t *ul = (*segtracker)[i]->ul_vector[j];
            if(ul) {
                if(ul->data)
                    delete ul->data;
                delete ul;
            }
        }
        (*segtracker)[i]->ul_vector.clear();
    }
    
    // Get rid of redo logs.
    for(size_t i=0; i<(*segtracker).size(); i++) {
        for(size_t j=0; j<(*segtracker)[i]->rl_vector.size(); j++) {
            redo_log_t *rl = (*segtracker)[i]->rl_vector[j];
            if(rl) {
                if(rl->data)
                    delete rl->data;
                delete rl;
            }
        }
        (*segtracker)[i]->ul_vector.clear();
    }

    delete segtracker;

    printf("INFO: Abort complete\n");
}

/*
 play through any committed or aborted items in the log file(s) and shrink the log file(s) as much as possible.
 
 TODO: Implement
*/
void rvm_truncate_log(rvm_t rvm) {
    printf("INFO: Truncating\n");
    std::map<std::string, segment_t*>::iterator itr;
    for(itr = rvm->seg_map.begin();itr != rvm->seg_map.end(); ++itr) {
        std::string segname = itr->first;
        // Load segment from file.
        std::string filename = rvm->dirpath + segname + ".seg";
        
        size_t seg_size;
        char *data = read_from_file(filename, &seg_size);

        std::vector<std::string> log_list = get_log_files(rvm->dirpath, segname);

        for(auto &lg : log_list) {
            // Apply log files to the segment.
            size_t k;
            char *log_data = read_from_file(rvm->dirpath + lg, &k);
            char *p = strchr(log_data, '\n');
            
            std::string s = "";
            char *q = log_data;
            while(q < p)
                s += *(q++);
            
            int offset = std::stoi(s);
            
            p = strchr(p+1, '\n');
            
            q++;
            
            s = "";
            
            while(q < p)
                s += *(q++);
            
            int size = std::stoi(s);
            
            k -= p-log_data;
            
            if(k != size) {
                std::cerr << "ERROR: Log may have been corrupted!" << std::endl;
                exit(1);
            }
            
            p++;
            
            for(int i=0; i<k; i++)
                (data+offset)[i] = p[i];           
        }

        // Write updated segment back to file.
        std::ofstream outputFile(filename);
        for(int i=0; i<seg_size; i++)
            outputFile << data[i];
        outputFile.close();

        // Delete log files
        delete_log_files(rvm->dirpath, segname);
    }
    
    printf("INFO: Truncate complete\n");
}
