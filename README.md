# About

A small Risc-V kernel with multithreading. University project.

# Dependencies (on Linux)

```
podman/docker (rootless is fine)
riscv64 toolchain (search your repos)
```

# Installing and running (on Linux)

1. Build the build container:

```
podman build --tag riscv-build .
```

You only need to do this once.
If using docker, edit the 2nd line of the Makefile

2. Build the kernel + user program + tests:

```
make
```

3. Run the kernel + user program + tests:

To just run everything:

```
make run
```

To run for debugging:

```
make run-gdb

# In another terminal:
gdb-multiarch
> target remote localhost:26000
```

To stop the emulator, press `Ctrl+a` and then `x`
