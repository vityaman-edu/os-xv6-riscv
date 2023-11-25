struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe; // FD_PIPE
  struct inode *ip;  // FD_INODE and FD_DEVICE
  UInt32 off;          // FD_INODE
  short major;       // FD_DEVICE
};

#define major(dev)  ((dev) >> 16 & 0xFFFF)
#define minor(dev)  ((dev) & 0xFFFF)
#define	mkdev(m,n)  ((UInt32)((m)<<16| (n)))

// in-memory copy of an inode
struct inode {
  UInt32 dev;           // Device number
  UInt32 inum;          // Inode number
  int ref;            // Reference count
  struct sleeplock lock; // protects everything below here
  int valid;          // inode has been read from disk?

  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;
  UInt32 size;
  UInt32 addrs[NDIRECT+1];
};

// map major device number to device functions.
struct devsw {
  int (*read)(int, UInt64, int);
  int (*write)(int, UInt64, int);
};

extern struct devsw devsw[];

#define CONSOLE 1
