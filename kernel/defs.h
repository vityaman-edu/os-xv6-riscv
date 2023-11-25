#include <kernel/types.h>

struct buf;
struct context;
struct file;
struct inode;
struct pipe;
struct proc;
struct spinlock;
struct sleeplock;
struct stat;
struct superblock;

// bio.c
void            binit(void);
struct buf*     bread(UInt32, UInt32);
void            brelse(struct buf*);
void            bwrite(struct buf*);
void            bpin(struct buf*);
void            bunpin(struct buf*);

// console.c
void            consoleinit(void);
void            consoleintr(int);
void            consputc(int);

// exec.c
int             exec(char*, char**);

// file.c
struct file*    filealloc(void);
void            fileclose(struct file*);
struct file*    filedup(struct file*);
void            fileinit(void);
int             fileread(struct file*, UInt64, int n);
int             filestat(struct file*, UInt64 addr);
int             filewrite(struct file*, UInt64, int n);

// fs.c
void            fsinit(int);
int             dirlink(struct inode*, char*, UInt32);
struct inode*   dirlookup(struct inode*, char*, UInt32*);
struct inode*   ialloc(UInt32, short);
struct inode*   idup(struct inode*);
void            iinit();
void            ilock(struct inode*);
void            iput(struct inode*);
void            iunlock(struct inode*);
void            iunlockput(struct inode*);
void            iupdate(struct inode*);
int             namecmp(const char*, const char*);
struct inode*   namei(char*);
struct inode*   nameiparent(char*, char*);
int             readi(struct inode*, int, UInt64, UInt32, UInt32);
void            stati(struct inode*, struct stat*);
int             writei(struct inode*, int, UInt64, UInt32, UInt32);
void            itrunc(struct inode*);

// ramdisk.c
void            ramdiskinit(void);
void            ramdiskintr(void);
void            ramdiskrw(struct buf*);

// kalloc.c
void*           kalloc(void);
void            kfree(void *);
void            kinit(void);

// log.c
void            initlog(int, struct superblock*);
void            log_write(struct buf*);
void            begin_op(void);
void            end_op(void);

// pipe.c
int             pipealloc(struct file**, struct file**);
void            pipeclose(struct pipe*, int);
int             piperead(struct pipe*, UInt64, int);
int             pipewrite(struct pipe*, UInt64, int);

// printf.c
void            printf(const char*, ...);
void            panic(const char*) __attribute__((noreturn));
void            printfinit(void);

// proc.c
int             cpuid(void);
void            exit(int);
int             fork(void);
int             growproc(int);
void            proc_mapstacks(pagetable_t);
pagetable_t     proc_pagetable(struct proc *);
void            proc_freepagetable(pagetable_t, UInt64);
int             kill(int);
int             killed(struct proc*);
void            setkilled(struct proc*);
struct cpu*     mycpu(void);
struct cpu*     getmycpu(void);
struct proc*    myproc();
void            procinit(void);
void            scheduler(void) __attribute__((noreturn));
void            sched(void);
void            sleep(void*, struct spinlock*);
void            userinit(void);
int             wait(UInt64);
void            wakeup(void*);
void            yield(void);
int             either_copyout(int user_dst, UInt64 dst, void *src, UInt64 len);
int             either_copyin(void *dst, int user_src, UInt64 src, UInt64 len);
void            procdump(void);
int             dump(void);
int             dump2(int pid, int reg_num, UInt64 ret_addr);

// swtch.S
void            swtch(struct context*, struct context*);

// spinlock.c
void            acquire(struct spinlock*);
int             holding(struct spinlock*);
void            initlock(struct spinlock*, char*);
void            release(struct spinlock*);
void            push_off(void);
void            pop_off(void);

// sleeplock.c
void            acquiresleep(struct sleeplock*);
void            releasesleep(struct sleeplock*);
int             holdingsleep(struct sleeplock*);
void            initsleeplock(struct sleeplock*, char*);

// string.c
int             memcmp(const void*, const void*, UInt32);
void*           memmove(void*, const void*, UInt32);
void*           memset(void*, int, UInt32);
char*           safestrcpy(char*, const char*, int);
int             strlen(const char*);
int             strncmp(const char*, const char*, UInt32);
char*           strncpy(char*, const char*, int);

// syscall.c
void            argint(int, int*);
int             argstr(int, char*, int);
void            argaddr(int, UInt64 *);
int             fetchstr(UInt64, char*, int);
int             fetchaddr(UInt64, UInt64*);
void            syscall();

// trap.c
extern UInt32     ticks;
void            trapinit(void);
void            trapinithart(void);
extern struct spinlock tickslock;
void            usertrapret(void);

// uart.c
void            uartinit(void);
void            uartintr(void);
void            uartputc(int);
void            uartputc_sync(int);
int             uartgetc(void);

// vm.c
void            kvminit(void);
void            kvminithart(void);
void            kvmmap(pagetable_t, UInt64, UInt64, UInt64, int);
int             mappages(pagetable_t, UInt64, UInt64, UInt64, int);
pagetable_t     uvmcreate(void);
void            uvmfirst(pagetable_t, UChar *, UInt32);
UInt64          uvmalloc(pagetable_t, UInt64, UInt64, int);
UInt64          uvmdealloc(pagetable_t, UInt64, UInt64);
int             uvmcopy(pagetable_t, pagetable_t, UInt64);
void            uvmfree(pagetable_t, UInt64);
void            uvmunmap(pagetable_t, UInt64, UInt64, int);
void            uvmclear(pagetable_t, UInt64);
pte_t *         walk(pagetable_t, UInt64, int);
UInt64          walkaddr(pagetable_t, UInt64);
int             copyout(pagetable_t, UInt64, char *, UInt64);
int             copyin(pagetable_t, char *, UInt64, UInt64);
int             copyinstr(pagetable_t, char *, UInt64, UInt64);

pte_t*          translate(pagetable_t, UInt64, int);

// plic.c
void            plicinit(void);
void            plicinithart(void);
int             plic_claim(void);
void            plic_complete(int);

// virtio_disk.c
void            virtio_disk_init(void);
void            virtio_disk_rw(struct buf *, int);
void            virtio_disk_intr(void);

// list.c
struct list {
  struct list* prev;
  struct list* next;
};

void            lst_init(struct list *lst);
int             lst_empty(struct list *lst);
void            lst_remove(struct list *e);
void*           lst_pop(struct list *lst);
void            lst_push(struct list *lst, void *p);
void            lst_print(struct list *lst);

// buddy.c
void            bd_init(void *base, void *end);
void*           bd_malloc(UInt64 nbytes);
void            bd_free(void *p);

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))
