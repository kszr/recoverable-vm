#include <map>
#include <vector>

typedef long trans_t;

//typedef int rvm_t;

struct undo_log_t {
	// const char* segname;
    char *segbase;
    char *data;
    int offset;
};

struct redo_log_t {
    char *segbase;
    char *metadata;
    int offset;
};

struct segment_t {
    char* segbase;
    //char *data;
    int ismapped;
    int busy;
    std::vector<undo_log_t*> ul_vector;
    redo_log_t *rl;
};

 struct rvm_s {
 	const char *dirpath;   // Directory that this rvm_t instance is responsible for.
    char *log_file;
    std::map<const char*, segment_t*> seg_map; // Keeps track of all segments maintained by this instance.
 };
 
 typedef rvm_s* rvm_t;


