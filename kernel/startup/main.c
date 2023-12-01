#include <kernel/core/param.h>
#include <kernel/core/type.h>
#include <kernel/defs.h>
#include <kernel/hw/memlayout.h>
#include <kernel/hw/arch/riscv/register.h>

volatile static int started = 0;

void show_greeting() {
  printf("\n");
  printf("+--------------------------+\n");
  printf("| Welcome to XV6 OS!       |\n");
  printf("| Edited by @vityaman.     |\n");
  printf("+--------------------------+\n");
  printf("\n");
}

// start() jumps here in supervisor mode on all CPUs.
void main() {
  if (cpuid() == 0) {
    consoleinit();
    printfinit();

    show_greeting();

    printf("[info][boot] Kernel is booting...\n");
    printf("[info][boot] Console initialzied.\n");
    printf("[info][boot] Printf initialized.\n");

    printf("[info][boot] Initializing frame allocator...\n");
    frame_allocator_init();

    printf("[info][boot] Creating kernel page table...\n");
    kvminit();

    printf("[info][boot] Turning on paging...\n");
    kvminithart();

    printf("[info][boot] Initializing proccesses...\n");
    procinit();

    printf("[info][boot] Preparing trap vectors...\n");
    trapinit();

    printf("[info][boot] Installing kernel trap vector...\n");
    trapinithart();

    printf("[info][boot] Setting up interup controller...\n");
    plicinit();

    printf("[info][boot] Asking PLIC for device interrupts...\n");
    plicinithart();

    printf("[info][boot] Initializing buffer cache...\n");
    binit();

    printf("[info][boot] Initializing inode table...\n");
    iinit();

    printf("[info][boot] Initializing file table...\n");
    fileinit();

    printf("[info][boot] Initializing emulated hard disk...\n");
    virtio_disk_init();

    printf("[info][boot] Initializing first user process...\n");
    userinit();

    __sync_synchronize();
    started = 1;

  } else {

    while (started == 0) {
      // Wait, do nothing
    }

    __sync_synchronize();
    printf("[info][boot][cpu %d] Hart is starting...\n", cpuid());

    printf("[info][boot][cpu %d] Turning on paging...\n", cpuid());
    kvminithart();

    printf("[info][boot][cpu %d] Installing kernel trap vector...\n", cpuid());
    trapinithart();

    printf("[info][boot][cpu %d] Asking PLIC for dev interrupts...\n", cpuid());
    plicinithart(); // ask PLIC for device interrupts
  }

  scheduler();
}
