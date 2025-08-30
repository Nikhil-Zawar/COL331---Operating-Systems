#define NPROC        64  // maximum number of processes
#define KSTACKSIZE 4096  // size of per-process kernel stack
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache


#define FSSIZE       7400  // size of file system in blocks (increased by 6400 to accomodate 800 swap pages)


#define SWAP_PAGES 800  // no of swap pages
#define SWAP_BLOCKS_PER_PAGE 8  // Swap blocks per page
#define SWAP_SIZE (SWAP_PAGES * SWAP_BLOCKS_PER_PAGE)   // Total size of swap space

