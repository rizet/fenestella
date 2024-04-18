# Testing
## Execution
### Host operating system
The provided testing and execution scripts invoked through the root Makefile are designed to be executed in a Linux-based development environment. If using any other operating system, then a virtual machine containing Linux, or an integrated environment such as Windows' Subsystem for Linux, must be configured in order to properly perform any compilation or virtualized execution of the operating system.

A temporary, standalone QEMU instance can be spawned from the command line using make. 

To do so, execute the command `make run` in the root of the repository. To append additional arguments to the scripted QEMU command for personal preference, append the previous command with the argument `ADD_QEMU_ARGS="<args>"` where `<args>` is substituted with the desired command options.

The spawned instance does not create any configuration files or storage devices, and the context of the instance is completely destroyed when the program terminates.

Similarly, a gdbserver-enabled QEMU instance can be spawned easily with the `make debug` command. Additional arguments are appended in the same fashion as before. The instance is configured to automatically open a gdbserver listening on port 1234, and the instance will not automatically start execution. The gdbserver-enabled instance will wait for either a gdb `continue` command or for the pause option to be disabled in the QEMU window.

### Dependencies
A prerequisite is to have `qemu-system-x86_64` and all its dependencies installed in the development environment before invoking the provided testing scripts. If the commands are not present, the Makefile script will fail to properly initialize any virtual instances.

## Debugging
The only supported method of debugging is through the QEMU gdbserver, which implicates any debugger used must be compliant with its interface: most namely, `gdb`. However, there are multiple frontends for interacting with `gdb`.

***NOTE:** connecting to QEMU via the address `localhost:1234` is only supported in environments that are running `gdb` in the same native Linux environment as the emulator instance. If running the debugger outside of a virtual machine or container, or in WSL, then the proper address for connection will differ, and the default address likely will not work.*

### `gdb` (command line)
To simply use the `gdb` command line, enter the root of the repository, generate a full system image, spawn a gdbserver-enabled QEMU instance in the background, and then execute `gdb`.

To debug with QEMU, you must attach `gdb` to QEMU's gdbserver. To do so, ensure the QEMU instance is open, and execute the debugger console command `target remote localhost:1234`.

To properly load debugging symbols, load either the `frame.se` or `glass.sys` ELF files located in the generated `build/` directory. In `gdb`, execute the command `file build/<ELF>` where `<ELF>` is the name of the intended targeted program.

To then start execution, type the `continue` command into the `gdb` console. It is not recommended to unpause execution through QEMU.
