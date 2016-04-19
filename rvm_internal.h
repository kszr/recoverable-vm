#include <map>
#include <vector>
#include <string>

typedef long trans_t; // This is so we can disguise addresses as trans_t.

struct undo_log_t {
	// const char* segname;
    char *segbase;
    char *data;
    int offset;
};

struct redo_log_t {
    char *segbase;
    char *data;
    int offset;
};

struct segment_t {
    std::string segname;
    std::string dirpath;
    size_t size;
    char* segbase;
    int ismapped;
    int busy;
    std::vector<undo_log_t*> ul_vector;
    std::vector<redo_log_t*> rl_vector;
};

 struct rvm_s {
 	std::string dirpath;   // Directory that this rvm_t instance is responsible for.
    char *log_file;
    std::map<const char*, segment_t*> seg_map; // Keeps track of all segments maintained by this instance.
 };
 
 typedef rvm_s* rvm_t; // This is so we can create objects on the heap in order to make them available between functions in a process.


