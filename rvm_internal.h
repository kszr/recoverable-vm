typedef int trans_t;
//typedef int rvm_t;

 struct rvm_t {
 	const char* dir_path;
        char* log_file;
 };

struct segment {
    char* segbase;
    //char *data;
    int ismapped;
    int busy;
};
	
struct undo_log{
	// const char* segname;
    char *segbase;
    char *data;
    int offset;
	// int size;
};

struct redo_log{
    char *segbase;
    char *metadata;
    int offset;
};


