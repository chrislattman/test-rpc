# RPC server

Use the provided Makefile to test the [RPC](https://en.wikipedia.org/wiki/Remote_procedure_call) server and client stubs.

This example leverages the `LD_PRELOAD` environment variable on Linux to hook the standard C library functions `open`, `close`, `read`, `write`, `lseek`, `stat`, `fstat`, and `fsync`. When a filename starts with "//", the function (either `open` or `stat`) arguments are marshaled/serialized and sent via TCP to a remote RPC server to be deserialized and executed there. Very large file descriptors (> 1,073,741,823) are assumed to have originated remotely, and functions that are passed in such file descriptors are likewise executed remotely. Results from each of the function calls are unmarshaled/serialized and sent back to the client via TCP.

Run `export RPC_HOST=<ip-address>` and/or `export RPC_PORT=<port-number>` as environment variables to specify custom values before running a remote test.

These function calls are all synchronous.
