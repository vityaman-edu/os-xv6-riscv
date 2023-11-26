K=kernel
U=user

OBJS = \
  $K/legacy/entry.o \
  $K/legacy/start.o \
  $K/legacy/console.o \
  $K/legacy/printf.o \
  $K/legacy/uart.o \
  $K/legacy/kalloc.o \
  $K/legacy/spinlock.o \
  $K/legacy/string.o \
  $K/legacy/main.o \
  $K/legacy/vm.o \
  $K/legacy/proc.o \
  $K/legacy/swtch.o \
  $K/legacy/trampoline.o \
  $K/legacy/trap.o \
  $K/legacy/syscall.o \
  $K/legacy/sysproc.o \
  $K/legacy/bio.o \
  $K/legacy/fs.o \
  $K/legacy/log.o \
  $K/legacy/sleeplock.o \
  $K/legacy/file.o \
  $K/legacy/pipe.o \
  $K/legacy/exec.o \
  $K/legacy/sysfile.o \
  $K/legacy/kernelvec.o \
  $K/legacy/plic.o \
  $K/legacy/virtio_disk.o\
  $K/legacy/list.o\
  $K/legacy/buddy.o\
	$K/cxxstd/malloc.o\
	$K/cxxstd/test.o\
	$K/modern/Bridge.o\
	$K/modern/memory/Address.o\
	$K/modern/memory/virt/AddressSpace.o\
	$K/modern/library/error/Panic.o\
	$K/modern/library/sync/Spinlock.o\
	$K/modern/memory/allocator/FrameAllocator.o\

# riscv64-unknown-elf- or riscv64-linux-gnu-
# perhaps in /opt/riscv/bin
TOOLPREFIX=sc-dt/riscv-gcc/bin/riscv64-unknown-elf-

# Try to infer the correct TOOLPREFIX if not set
ifndef TOOLPREFIX
TOOLPREFIX := $(shell if riscv64-unknown-elf-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-elf-'; \
	elif riscv64-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-linux-gnu-'; \
	elif riscv64-unknown-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-linux-gnu-'; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find a riscv64 version of GCC/binutils." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

QEMU = qemu-system-riscv64

CC = $(TOOLPREFIX)gcc
CXX = $(TOOLPREFIX)g++
AS = $(TOOLPREFIX)as
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -Wall -Werror -Wextra 
CFLAGS += -O -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -MD
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -I.
# CFLAGS += -std=c2x
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

CXXFLAGS = -Wall -Werror -Wextra 
CXXFLAGS += -O -fno-omit-frame-pointer -ggdb -gdwarf-2
CXXFLAGS += -MD
CXXFLAGS += -mcmodel=medany
CXXFLAGS += -ffreestanding -fno-common -mno-relax
CXXFLAGS += -I.
CXXFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
# CXXFLAGS += -nostdlib -nostdinc -nostdinc++ 
CXXFLAGS += -fno-rtti -fno-exceptions 
CXXFLAGS += -fno-unwind-tables -fno-asynchronous-unwind-tables

# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
CXXFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
CXXFLAGS += -fno-pie -no-pie
endif

LDFLAGS = -z max-page-size=4096

$K/kernel: $(OBJS) $K/legacy/kernel.ld $U/initcode
	$(LD) $(LDFLAGS) -T $K/legacy/kernel.ld -o $K/kernel $(OBJS) 
	$(OBJDUMP) -S $K/kernel > $K/kernel.asm
	$(OBJDUMP) -t $K/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym

$U/initcode: $U/initcode.S
	$(CC) $(CFLAGS) -march=rv64g -nostdinc -I. -Ikernel -c $U/initcode.S -o $U/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $U/initcode.out $U/initcode.o
	$(OBJCOPY) -S -O binary $U/initcode.out $U/initcode
	$(OBJDUMP) -S $U/initcode.o > $U/initcode.asm

tags: $(OBJS) _init
	etags *.S *.c

ULIB = $U/ulib.o $U/usys.o $U/printf.o $U/umalloc.o

_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -T $U/user.ld -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

$U/usys.S : $U/usys.pl
	perl $U/usys.pl > $U/usys.S

$U/usys.o : $U/usys.S
	$(CC) $(CFLAGS) -c -o $U/usys.o $U/usys.S

$U/_forktest: $U/forktest.o $(ULIB)
	# forktest has less library code linked in - needs to be small
	# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $U/_forktest $U/forktest.o $U/ulib.o $U/usys.o
	$(OBJDUMP) -S $U/_forktest > $U/forktest.asm

$U/dumptests.o: $U/dumptests.S $U/dumptests.c $K/legacy/syscall.h
	$(CC) $(CFLAGS) -c -o $U/dumptests.s.o $U/dumptests.S
	$(CC) $(CFLAGS) -c -o $U/dumptests.c.o $U/dumptests.c
	$(LD) -r $U/dumptests.c.o $U/dumptests.s.o -o $U/dumptests.o

$U/dump2tests.o: $U/dump2tests.S $U/dump2tests.c $K/legacy/syscall.h
	$(CC) $(CFLAGS) -c -o $U/dump2tests.s.o $U/dump2tests.S
	$(CC) $(CFLAGS) -c -o $U/dump2tests.c.o $U/dump2tests.c
	$(LD) -r $U/dump2tests.c.o $U/dump2tests.s.o -o $U/dump2tests.o

mkfs/mkfs: mkfs/mkfs.c $K/legacy/fs.h $K/legacy/param.h
	gcc -Werror -Wall -I. -o mkfs/mkfs mkfs/mkfs.c

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o

UPROGS=\
	$U/_cat\
	$U/_echo\
	$U/_forktest\
	$U/_grep\
	$U/_init\
	$U/_kill\
	$U/_ln\
	$U/_ls\
	$U/_mkdir\
	$U/_rm\
	$U/_sh\
	$U/_stressfs\
	$U/_usertests\
	$U/_grind\
	$U/_wc\
	$U/_zombie\
	$U/_pingpong\
	$U/_dumptests\
	$U/_dump2tests\
	$U/_alloctest\
	$U/_cowtest\

fs.img: mkfs/mkfs README $(UPROGS)
	mkfs/mkfs fs.img README $(UPROGS)

-include kernel/*.d user/*.d

clean:
	rm -rf */*/*.o */*/*.d
	rm -rf */*/*/*.o */*/*/*.d
	rm -rf */*/*/*/*.o */*/*/*/*.d
	rm -rf */*/*/*/*/*.o */*/*/*/*/*.d
	rm -rf */*/*/*/*/*/*.o */*/*/*/*/*/*.d
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg \
	*/*.o */*.d */*.asm */*.sym \
	$U/initcode $U/initcode.out $K/kernel fs.img \
	mkfs/mkfs .gdbinit \
        $U/usys.S \
	$(UPROGS)

# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS
CPUS := 3
endif

QEMUOPTS = -machine virt -bios none -kernel $K/kernel -m 128M -smp $(CPUS) -nographic
QEMUOPTS += -global virtio-mmio.force-legacy=false
QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

qemu: $K/kernel fs.img
	$(QEMU) $(QEMUOPTS)

.gdbinit: .gdbinit.tmpl-riscv
	sed "s/:1234/:$(GDBPORT)/" < $^ > $@

qemu-gdb: $K/kernel .gdbinit fs.img
	@echo "*** Now run 'gdb' in another window." 1>&2
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)

