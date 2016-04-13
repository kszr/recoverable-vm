# recoverable-vm
redo log will be a vector or linked list.

about to modify should create the undo log copying the segment to be modified

on commit you write everything to redo log(not pushed to disk here)

destroy should also call unmap for a sanity check
