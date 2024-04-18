# Skylight

### Philosophy
As defined by [dictionary.com](https://www.dictionary.com/browse/skylight#), a "skylight" is:
> an opening in a roof or ceiling, fitted with glass, for admitting daylight.

Skylight is an operating system that is uniquely modeled, and aims to have consistent, clean, thorough, transparent, and easy-to-read code, in order to help other developers understand its operation and general principles of operating systems with a more clear-cut exposition.

### Compilation
Skylight relies on Linux software to compile, therefore any build process must take place under native Linux, emulated Linux, or the Windows Subsystem for Linux.

Additionally, the development environment requires these packages:
```
util-linux 
mtools 
nasm 
clang 
build-essential 
wget
```

In order to compile, simply enter the root of the repository, and run `make image`. A FAT32-formatted hard drive image will be generated. The `glass.sys` and `frame.se` executables are located in the generated `build/` directory to facillitate loading debugger symbols, and each individual `.o` file can also be found in the same directory. For more information on build dependencies and their specific versions, see [BUILD-DEPS](docs/BUILD-DEPS.md).

### Execution
In order to test the image, the testing emulator/hardware must be configured to support:
 - x86-64, with PAE-NX
 - UEFI v2.0 (minimum)
 - PCI Express
 - 512MiB RAM (minimum)

To quickly test in a temporary QEMU instance, enter the root of the repository after generating an image, and execute `make run`.

To test on real hardware, meet the requirements above, and write the image to a drive. Select the Limine/skylight boot option through the UEFI boot menu. The system will not boot on a computer that has Secure Boot enabled.

For more information, reference [TESTING](docs/TESTING.md).

### License
As of 06/18/2022, the license on this repository has been updated to the MIT license. However, any commits prior to commit [`47912c3`](https://github.com/austanss/skylight/commit/47912c3e3d6b84a53ded0a549ef881042b5731ac) still fall under the Creative Commons public domain license applied at the time.

For more information on the terms of the MIT license, see the [LICENSE file](LICENSE).

### Legal
Licenses of certain external source files and binaries included in this repository must be visibly reproduced in the source code or in external documentation. To view these reproduced licenses, see [LICENSES](docs/LICENSES.md).

## Acknowledgements

### Development Cycle
OS development is... time consuming. I have other things to dedicate my time to, so I can't always work on Skylight. I will try to push at least one commit every week, but I can't guarantee that I will be able to do so. In summer months you can expect a high volume of commits. December is also a rather popular month for me. I have been working on this operating system project since mid-2021, and I don't intend on abandoning it. Just some times, it's a literal pain to work on. Other times, I don't have the time to do it.

### Contributing
I largely do not take contributions. I do take suggestions and feedback, but most of the source code should be written by me. If the project plateaus and the work is no longer worth my dedication I may open it up to contributions to keep it alive, but for now, it's principally my effort.

### Issues
If an issue is opened on this repository, I cannot promise my full attention to its resolution. I probably will read it and give a response if I am able to articulate one with the given information, but unless a presented issue is well-debugged and well-articulated I cannot swear that the issue will see a proper resolution.
