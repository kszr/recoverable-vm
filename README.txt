BASIC INFORMATION

* Segments are backed up on the disk as .seg files, which contain pure data.
* When creating a new segment that is not present on the disk, a new .seg file is initialized with a string of NULL characters of the requested length.

LOGS

* We use two types of logs: undo logs and redo logs.
* We follow a twofold truncation strategy:
    (i) We truncate existing logs whenever a new rvm object is created based on a given directory. This ensures that the segments on the disk are only updated lazily after a transaction has been committed (see (ii) for exception) and also that the data in the backing file of a given segment is up to date whenever another process decides to create an rvm object based on the same directory. We rely for the accuracy and freshness of segment data on the fact that our recoverable virtual memory system does not need to concern itself with concurrency, and that it is the user's responsibility to program defensively and only run processes that access the same rvm directory in sequential order.
    (ii) We also truncate in rvm_commit_transaction() whenever the number of log files has reached or crossed a certain threshold (set to 10 for the time being in rvm_internal.h). This ensures that segments are updated on the disk at intervals when a process executes a large number of transactions, and reduces latency incurred in (i) when a new process tries to access the data on the segment in the future.

UNDO LOG

* Undo logs are not backed up on disk, but are instead kept in a queue data structure.
* An undo log is created by storing a snapshot of the region that the user wishes to modify within each segment specified in a call to rvm_about_to_modify().
* We don't check to see whether the user has actually made any changes between calls to rvm_about_to_modify() within a given transaction, but simply create undo logs for each call.
* We also don't check to see whether the user has changed any other region of a segment than that declared in a call to rvm_about_to_modify(). This does not pose a problem for us for two reasons:
    1. If deceptive changes were made to an undeclared region of a segment, then the change will simply not get logged in a redo log when the transaction is committed, since redo logs are only concerned with regions that were declared in calls to about_to_modify.
    2. We assume that declared regions in successive calls to about_to_modify() within a transaction accumulate. In other words, once a user calls about_to_modify() on a given region, there is no limit to the number of modifications he or she may make to that region.

REDO LOG

* A redo log is structurally similar to an undo log, with the difference that it records changes that the user has made to a region of a segment and is in addition backed on the disk in the form of .log files.
* We record as many redo logs as there are undo logs in a given transaction - which is to say, we record a redo log corresponding to each call to about_to_modify(). This is in contrast with the LRVM paper, which continually appends just one redo log and truncates it at intervals. We went with our approach because the assignment stated, "It is up to you how many log files to use and what specific information to write to them."
* The redo logs themselves are generated only in rvm_commit_transaction() and are generated in the order in which modifications were made to a segment - i.e., in the order in which undo logs were made. This is straightforward in the case in which a transaction consists of modifications to non-overlapping regions in a segment. In the case in which modifications are made to overlapping regions of the segment, on the other hand, the redo log does not faithfully record the exact modifications that were made - only the final data in the region of the segment that was declared in a call to about_to_modify(). E.g., if regions A and B overlap, and the user first writes aaaaa to region A and bbbbb to region B, the redo logs won't record these modifications exactly, but may record something like aaabb and bbbbb. This is not a problem, because the final data in A U B will match the user's expectations when the logs are applied in chronological order. Moreover, transactions are by definition atomic, so the intermediate state of the data in a transaction represented by the log files can take any form as long as the final data reflects the modifications made by the user.
* On commit, we write each redo log to disk. Log files are named according to the segments they represent, and contain the offset of the region within the segment and the number of bytes in the region, in addition to the actual data recorded by the log file in the region.
* On truncate, log files are read in chronological order and applied to the region of the segment represented by the offset.