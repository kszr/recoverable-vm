typedef int trans_t;
typedef int rvm_t;

// struct rvm {
// 	const char* dir_path;
//     rvm_t id;
// };

struct segment {
	char* segaddr;
	bool ismapped;
};
	
struct undo_log{
	const char* segname;
	int offset;
	int size;
} undo_log;

struct redo_log{
	const char* segname;
	int offset;
	int size;
	
} redo_log;


