#include <map>
#include <vector>
#include <string>

#define TRUNCATE_THRESHOLD 10  // System should truncate logs after this many commits.

typedef long trans_t; // This is so we can disguise addresses as trans_t.

struct undo_log_t {
	// const char* segname;
    char *segbase;
    char *data;
    size_t size;
    int redone;
    int offset;
};

struct redo_log_t {
    char *segbase;
    char *data;
    size_t size;
    int offset;
};

struct rvm_s;
typedef rvm_s* rvm_t;

struct segment_t {
    std::string segname;
    std::string dirpath;
    size_t size;
    char* segbase;
    int ismapped;
    int busy;
    rvm_t rvm; // Stores a reference to the rvm object that owns this segment.
    std::vector<undo_log_t*> ul_vector;
    std::vector<redo_log_t*> rl_vector;
};

 struct rvm_s {
 	std::string dirpath;   // Directory that this rvm_t instance is responsible for.
    char *log_file;
    std::map<std::string, segment_t*> seg_map; // Keeps track of all segments maintained by this instance.
    size_t num_commits; // The number of redo logs that have been written between calls to rvm_truncate()
 };
 
 typedef rvm_s* rvm_t; // This is so we can create objects on the heap in order to make them available between functions in a process.


