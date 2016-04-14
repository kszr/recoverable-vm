typedef int trans_t;
typedef int rvm_t;

typedef struct rvm_s {
	const char* dir_path;
    rvm_t id;
} rvm;

typedef struct segment_s {
	const char* segaddr;
	bool kill;
} segment;
	
typedef struct undo_log{
	const char* segname;
	int offset;
	int size;
} undo_log;

typedef struct redo_log{
	const char* segname;
	int offset;
	int size;
	
} redo_log;


