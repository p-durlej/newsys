# Nameless OS Lite

This is the complete source code of Nameless OS Lite v1.2.3, it's around 92K LoC.

The system is released under a BSD 2-clause license.

![Nameless OS v1.2.3 desktop with apps](http://www.durlej.net/newsys/2016-v12.png)

## Running

If you want to run the operating system without building it, there are USB stick, CDROM ISO and PXE images available for download at GitHub.
You can install the operating system if you want, but you don't have to. There is a live USB/CD/PXE mode.

## Building

The current version of the operating system was tested to build successfully on both OpenBSD 6.0 and Ubuntu 16.04 LTS.

Before the system can be compiled from source, you should make sure that you have gmake in your $PATH. If you intend to run the operating system using the scripts in the "s" directory, provide a qemu command in your search path. Symbolic links to appropriate programs will suffice.

Also, to compile the GCC toolchain, you must have the mpfr and gmp libraries. If the host OS makes a distinction, these should be the development versions.

The source code should be placed at $HOME/os386/src. Other locations may work, but were not tested recently.

### Building everything

To start the build, change the current working directory to $HOME/os386/src and run the s/mkall script. This should download the gcc and binutils source tarballs, build and install the toolchain in $HOME/os386/tools, then build i386 and amd64 releases and finally install the library and header files in the toolchain directories.

The resultant release tarballs are placed in $HOME/os386/src.

### Building a single platform release

Building the entire toolchain and the two platforms can be time consuming, it takes 10 - 40 minutes on my machines, so if you just want to build a single platform release, you can run ARCH=amd64 s/mkrelse in $HOME/os386/src. If you want to build the i386 release, you can substitute i386 for amd64. Amd64 is the default, so you can also omit ARCH=amd64.

### Rebuilding after making changes

If you just made a change in the source code and want to recompile, you should firstly source some environment variables into the current shell by running . s/env.sh

If the $HOME/os386/src tree does not include the required symlinks, run make symlinks or s/mkrelse.

Then just run the gmake command.

Of course the toolchain is required. Again, amd64 is built by default. If you want to build i386 instead, set ARCH=i386 in the environment and run make symlinks or s/mkrelse.

## Program development

### API

There is currently no API documentation available.

Therefore the kernel, driver and library source code is the documentation and the bundled applications are the examples ;-)

Standard library functions such as printf, act much like their standard counterparts from "big" operating systems. Nonetheless, this is a hobby OS and some parts of the implementation are simplified.

Some standard functions are reduced in functionality, some do not exist at all. There is a race condition in open, iirc. Vfprintf and the other printf functions do not support floating point conversions, and \<math.h\> is missing from this version.

Some functions are Nameless OS-specific, these can be figured out by reading the source code.

The GUI API is completely made by up me, there isn't any similar API documented anywhere. Again, the source code is the documentation.

### Compiling programs

Programs for Nameless OS are built using the standard GNU tools.

The executable prefix is either amd64-os386-elf- or i386-os386-elf- depending on the platform. So, if you want to compile a single-file program, you can run amd64-os386-elf-gcc -o hello hello.c

Just make sure the $HOME/os386/tools/bin directory is in your $PATH.
