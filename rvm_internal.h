#include <map>

typedef long trans_t;
//typedef int rvm_t;

struct segment_t {
    char* segbase;
    //char *data;
    int ismapped;
    int busy;
};

 struct rvm_t {
 	const char *dirpath;   // Directory that this rvm_t instance is responsible for.
    char *log_file;
    std::map<const char*, segment_t*> seg_map; // Keeps track of all segments maintained by this instance.
 };
	
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


