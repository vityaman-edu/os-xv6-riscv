# Installation guide of xv6-riscv operating system

It is recommended to use Ubuntu 18.04 or Ubuntu 20.04

## Install QEMU

```bash
cd qemu

export QEMU_ARTIFACT=qemu-8.1.0

wget https://download.qemu.org/$QEMU_ARTIFACT.tar.xz
tar xvJf $QEMU_ARTIFACT.tar.xz

cd $QEMU_ARTIFACT
  ./configure --target-list=riscv64-softmmu
  cd build
    make && make install
  cd ..
cd ..

qemu-system-riscv64 --help
```

## Install RISC Toolchain by Syntacore

1. Download <https://drive.google.com/file/d/1T501e1kVMo0KPMUMsmlZ1P4D-Hxser8i/view?usp=drive_link>

2. Unpack the archive, copy `sc-dt` to project root

3. Add `sc-dt` to `.gitignore`

4. Change a `TOOLPREFIX` inside make file to be `TOOLPREFIX=sc-dt/riscv-gcc/bin/riscv64-unknown-elf-`

## Make the project for QEMU

```bash
make clean
make qemu
```

## Exit

```bash
Ctrl-A + X
```
