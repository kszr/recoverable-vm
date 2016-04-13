

typedef int trans_t;
typedef int rvm_t;

typedef struct rvm {
	const char* directory path;
	
};

typedef struct undo_log{
	const char* segname;
	int offset;
	int size;
	

}undo_log;

typedef struct redo_log{
	const char* segname;
	int offset;
	int size;
	
}redo_log;


