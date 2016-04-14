typedef int trans_t;
typedef int rvm_t;

// struct rvm {
// 	const char* dir_path;
//     rvm_t id;
// };

struct segment {
	char* segaddr;
    char *data;
	bool ismapped;
	bool busy;
};
	
struct undo_log{
	// const char* segname;
    char *segbase;
    char *data;
	int offset;
	// int size;
};

struct redo_log{
	const char* segname;
	int offset;
	int size;
	
};


