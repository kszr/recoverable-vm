BASIC INFORMATION

* Segments are backed up on the disk as .seg files, which contain pure data.
* When creating a new segment that is not present on the disk, a new .seg file is initialized with a string of NULL characters of the requested length.

LOGS

* We use two types of logs: undo logs and redo logs.

UNDO LOG

* Undo logs are not backed up on disk, but are instead kept in a queue data structure.
* An undo log is created by storing a snapshot of the region that the user wishes to modify within each segment specified in a call to rvm_about_to_modify().
* We don't check to see whether the user has actually made any changes between calls to rvm_about_to_modify() within a given transaction, but simply create undo logs for each call.
* We also don't check to see whether the user has changed any other region of a segment than that declared in a call to rvm_about_to_modify(). This does not pose a problem for us, since any such deceptive act on the part of the user will not get logged in a redo log and will not be written to disk by the very nature of our logging mechanism.

REDO LOG

* A redo log is structurally similar to an undo log, with the difference that it records changes that the user has made to a region of a segment and is in addition backed on the disk in the form of .log files.
* We record as many redo logs as there are undo logs in a given transaction - which is to say, we record a redo log corresponding to each call to about_to_modify().
* The redo logs themselves are generated only in rvm_commit_transaction() and are generated in the order in which modifications were made to a segment - i.e., in the order in which undo logs were made. This is straightforward in the case in which a transaction consists of modifications to non-overlapping regions in a segment. In the case in which modifications are made to overlapping regions of the segment, on the other hand, the redo log does not faithfully record the exact modifications that were made - only the final data in the region of the segment that was declared in a call to about_to_modify(). E.g., if regions A and B overlap, and the user first writes aaaaa to region A and bbbbb to region B, the redo logs won't record these modifications exactly, but may record something like aaabb and bbbbb. This is not a problem, because the final data in A U B will match the user's expectations when the logs are applied in chronological order. Moreover, transactions are by definition atomic, so the intermediate state of the data in a transaction represented by the log files can take any form as long as the final data reflects the modifications made by the user.
* On commit, we write each redo log to disk. Log files are named according to the segments they represent, and contain the offset of the region within the segment and the number of bytes in the region, in addition to the actual data recorded by the log file in the region.
* On truncate, log files are read in chronological order and applied to the region of the segment represented by the offset.