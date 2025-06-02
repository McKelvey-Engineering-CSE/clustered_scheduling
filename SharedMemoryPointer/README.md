# SharedMemoryPointer

This module is designed to provide a shared memory pointer to replace the single-use-barrier in the original clustering launcher, but can also be used as shared data. The smart pointer now can automatically handle the initilization and cleanup of the shared memory segment, against unexceptional signals as well (except SIGKILL). The process_primitives.* has been edited to allow robust mutex to enable issues with a dead lock holder.

To use it, see test.cpp.
