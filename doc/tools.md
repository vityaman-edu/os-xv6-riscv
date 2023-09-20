# Toolchain Setup

## Clangd

Install and run the `bear` utility
to generate `compile_commands.json`.

```bash
sudo dnf install -y bear
make clean
bear -- make qemu
```

Then you will see this file and `.clang`
will work properly.
