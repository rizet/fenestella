# Build Dependencies
## Operating system
The provided testing and execution scripts invoked through the root Makefile are designed to be executed in a Linux-based development environment. If using any other operating system, then a virtual machine containing Linux, or an integrated environment such as Windows' Subsystem for Linux, must be configured in order to properly perform any compilation or virtualized execution of the operating system.
## Programs
### nasm
To build Skylight, NASM is required to compile the assembly source files. The files are written in NASM assembler syntax, so it is not possible to successfully compile this repository with any other assembler.
### clang
Skylight builds with the latest LLVM, and uses compiler-specific features and syntax. However, Skylight also compiles with `-pedantic` so there is a bit of backwards version compatibility. It is not recommended to use any version earlier than `clang-7`.
### mtools
Simply install mtools in order to format the FAT32 image, no version compatibility issues have been detected.
### dosfstools
Similar versioning as mtools, is used similarly to help format the FAT32 image.
### wget
This is probably already installed on most Linux distributions, but it is required in order to obtain the latest Limine bootloader image during creation of the OS image.
### util-linux
This package is almost definitely installed on all Linux distributions, as it is depended on by many core programs. However, if it is not installed, it is required to install it for the purposes of compilation.
### build-essential
This package has different names on different distributions. `build-essential` is its name on Ubuntu. However, most distributions have a variation of this package which install basic development tools such as make, git, or compilers. Install this package regardless of which distribution you are using.