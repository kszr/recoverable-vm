BASIC INFORMATION

* Segments are backed up on the disk as .seg files, which contain pure data.
* When creating a new segment that is not present on the disk, a new .seg file is initialized with a string of NULL characters of the requested length.

LOGS

* We use two types of logs: undo logs and redo logs.

UNDO LOG

* Undo logs are not backed up on the disk, but is instead kept in a queue data structure.
* An undo log is created by storing a snapshot of the region that the user wishes to modify within each segment specified in a call to rvm_about_to_modify().
* We don't check to see whether the user has actually made any changes between calls to rvm_about_to_modify() within a given transaction, and simply create undo logs for each call.
* We also don't check to see whether the user has changed any other region of a segment than that declared in a call to rvm_about_to_modify(). This does not pose a problem for us, since any such deceptive act on the part of the user will not get logged in a redo log and will not be written to disk by the very nature of our logging mechanism.

REDO LOG

* A redo log is structurally similar to an undo log, with the difference that it records changes that the user has made to a region of a segment and is in addition backed on the disk in the form of .log files.
* Redo logs are centered on calls to rvm_about_to_modify() and rvm_commit_transaction().
* A redo log takes a snapshot of a region of a segment that would have previously been recorded in an undo log when a call to rvm_about_to_modify() was made.
* We simply use the offset, segment, and size information from the last undo log and record the data currently present in that region of a segment.
* This is done in rvm_about_to_modify(). The assumption we make is that the user would have already made any changes that were declared in an earlier call to rvm_about_to_modify(), if any. <<<<<<<<< THIS IS SUSPECT <<<<<<<<<
* In addition, each undo log can have at most one redo log associated with it (via o)

< TODO : COMPLETE >