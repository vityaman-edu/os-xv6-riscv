struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  UInt32 dev;
  UInt32 blockno;
  struct sleeplock lock;
  UInt32 refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  UChar data[BSIZE];
};

