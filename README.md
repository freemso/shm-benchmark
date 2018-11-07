# Benchmark on Shared Memory and Semaphore
## A Brief Discussion
### Semaphore
A semaphore is a counter used to provide access to a shared data object for multiple processes.

To obtain a shared resource, a process needs to do the following:

1. Test the semaphore that controls the resource.
2. If the value of the semaphore is positive, the process can use the resource. In this case, the process decrements the semaphore value by 1, indicating that it has used one unit of the resource.
3. Otherwise, if the value of the semaphore is 0, the process goes to sleep until the semaphore value is greater than 0. When the process wakes up, it return to step 1.

When a process is done with a shared resource that is controlled by a semaphore, the semaphore value is incremented by 1. If any other processes are asleep, waiting for the semaphore, they are awakened.

To implement semaphore correctly, the test of a semaphore’s value and the decrementing of this value must be an atomic operation. For this reason, semaphores are normally implemented inside the kernel.

A common form of semaphore is called a binary semaphore. It controls a single resource, and its value is initialized to 1. In general, however, a semaphore can be initialized to any positive value, with the value indicating how many units of the shared resource are available for sharing.

### Share Memory
Shared memory allows two or more processes to share a given region of memory. This is the fastest form of IPC, because the data does not need to be copied between the client and the server. The only trick in using shared memory is synchronizing access to a given amount multiple processes. If the server is placing data into a shared memory region, the client shouldn’t try to access the data until the server is done. Often semaphores are used to synchronize shared memory access.

The XSI shared memory differs from memory-mapped files in that there is no associated file. The XSI shared memory segments are anonymous segments of memory.

The kernel maintains a structure with at least the following members for each shared memory segment:

```c
struct shmid_ds {
	struct ipc_perm		shm_perm;
	size_t				shm_segsz;
	pid_t				shm_lpid;
	pid_t				shm_cpid;
	shmatt_t			shm_nattch;
	time_t 				shm_atime;
	time_t 				shm_dtime;
	time_t				shm_ctime;
};
```

The first function called is usually `shmget`, to obtain a shred memory identifier.

```c
#include <sys/shm.h>

int shmhet(key_t key, size_t size, int flag);
```

The function above returns shared memory id if everything is ok, `-1` if on error.

The `size` parameter is the size of the shared memory segment in bytes. Implementations will usually round up this size to a multiple of the system’s page size, but if an application specifies `size` as a value other than an integral multiple of the system’s page size, the remainder of the last page will be unavailable for use. If a new segment is being created, we must specify its `size`. If we are referencing an existing segment, we can specify `size` as 0. When a new segment is created, the contents of the segment are initialized with zeros.

Once a shared memory segment has been created, a process attaches it to its address space by calling `shmat`.

```c
#include <sys/shm.h>

void *shmat(int shmid, const void *addr, int flag);
```

The function above returns pointer to shared memory if ok, `-1` if on error.

The address in the calling process at which the segment is attached depends on the `addr` argument and whether the `SHM_RND` bit is specified in `flag`.

Unless we plan to run the application on only a single type of hardware (which is highly unlikely today), we should not specify the address where the segment is to be attached. Instead, we should specify an `addr` of `0` and let the system choose the address.

## Implementation Details
We use two semaphores to make sure the two processes do their jobs in a decent manner. The first semaphore could be see as `writable` indicating whether the shared memory is writable or not and the second is called `readable` indicating whether the memory is readable or not.

At initial, `writable` is set to `1` meaning that the memory is empty and ready to be written by the writer. Accordingly, `readable` is set to `0`.

Whenever the writer tries to write to the buffer, it will first try to decrease `writable`, getting the *write lock* in other words. After successful writes, the writer will increase `readable`, signaling that the memory is now full and ready to be read. Similarly, the reader does the same thing we it tries to read but only swap the two semaphores.

By using two semaphores, we make sure that whenever the writer writes to the memory, the old data has already been read thus leads to no data loss. And whenever the reader reads from the memory, the data in memory is always the new one preventing from duplicated read.

## Benchmark
We did some performance tests on Ubuntu 16.04 running in a VirtualBox 5.2.8 on my MacBook Pro (13-inch, 2017) with one 2.3 GHz Intel Core i5 processor and 2048 MB memory.

We implement two programs that can copy the content of a file to a another file. In the first program, `shm.c`, we use two processes to finish the job. One process reads from the source file and another writes to the target file. Two processes communicates through shared memory. And the buffer size of the memory is configurable through argument of the program.

To compare to performance, we also implement another program, `cpy.c`, that copy file using only one process.

Both `shm.c` and `cpy.c` support file size larger than buffer size. **(bonus)**

We test time cost of two programs to copy the file in various file size(`{16MB, 64MB, 1G}`) and buffer size(`{16KB, 64KB, 256KB}`).

The test results are shown in `output/shm.txt` and `output/cpy.txt`.

## Result Analysis
We can see that using two processes with shared memory do have some performance impact compared to only one process. But the impact is so small that we can almost ignore that. This shows that shared memory is very fast compared with other IPC, because the two processes could directly read and write the same memory without additional system calls.