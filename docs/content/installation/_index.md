---
weight: 20
title: Installation
---

## Environments

Currently libcsp only supports environments listed below:

- Language: C
- Compiler: GCC(>=8)
- Architecture: x86_64
- OS: Linux

## Installation

### Step 1: install GCC plugin development tools

To build libcsp, you need to install GCC plugin development tools first. The
installing command depends on your package manager,

- apt:

```shell
$ sudo apt install gcc-`gcc -dumpversion | awk -F . '{print $1}'`-plugin-dev
```

- yum:

```shell
$ sudo yum install -y gcc-plugin-devel.x86_64
```

### Step 2: build and install libcsp

Download the latest source from [here](https://github.com/shiyanhui/libcsp/releases). Then

```shell
$ tar -zxvf libcsp-x.y.z.tar.gz
$ cd libcsp-x.y.z
$ ./configure
$ make
$ sudo make install
```

Libcsp provides several options for `./congiure`:

- `--enable-debug`: It will disable the gcc optimization and add debug information if enabled.
- `--enable-valgrind`: It will add support for `valgrind` if enabled.
- `--with-sysmalloc`: It will use system's `malloc` method when malloc the process stack if enabled.

Use variables `CC` and `CXX` to explicitly control which GCC version you use.

Example:

```shell
$ ./configure CC=gcc-8 CXX=g++-8 --enable-debug --enable-valgrind
```
