//buffer 
struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf? (does disk sync to buff)
  uint dev;    // specifies the device to which this block belongs
  uint blockno; // block number on disk
  struct sleeplock lock; //lock to prevent other process to uses this buffer
  uint refcnt;  // reference counter, indicating the number of processes currently using this buffer.
  struct buf *prev; // LRU cache list
  struct buf *next; // pointer to next buffer
  uchar data[BSIZE]; //data
};

