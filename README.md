# RPC client and server

Use the provided Makefile to test the [RPC](https://en.wikipedia.org/wiki/Remote_procedure_call) server and client.

This example leverages the `LD_PRELOAD` environment variable on Linux to hook the standard C library functions `open`, `close`, `read`, `write`, `lseek`, `truncate`, `ftruncate`, `stat`, `fstat`, `fsync`, `rename`, and `unlink` using a [shim](https://en.wikipedia.org/wiki/Shim_(computing)) (rpc_hooks.c). When a filename starts with "//", the function (`open`, `truncate`, `stat`, `rename`, or `unlink`) arguments are marshaled/serialized and sent via TCP to a remote RPC server to be unmarshaled/deserialized and executed there. Very large file descriptors (> 1,073,741,823) are assumed to have originated remotely, and functions that are passed in such file descriptors are likewise executed remotely, with their function arguments marshaled/serialized before transmission. Results from each of the function calls are marshaled/serialized in the server and sent back to the client via TCP, where they are subsequently unmarshaled/deserialized.

- On macOS, the `DYLD_INSERT_LIBRARIES` environment variable serves the same purpose as `LD_PRELOAD` on Linux; however, the executable needs to be compiled using Apple clang with the flag `-flat_namespace`

Run `export RPC_HOST=<ip-address>` and/or `export RPC_PORT=<port-number>` as environment variables to specify custom values before running a remote test.

These function calls are all synchronous.

These examples all communicate with servers written in the same language. Well-known language-agnostic RPC frameworks include [gRPC](https://en.wikipedia.org/wiki/GRPC) and [Cap'n Proto](https://en.wikipedia.org/wiki/Cap%27n_Proto).

- These frameworks use their own data serialization formats
    - gRPC uses [Protocol Buffers (Protobuf)](https://en.wikipedia.org/wiki/Protocol_Buffers)
    - Another format is [FlatBuffers](https://en.wikipedia.org/wiki/FlatBuffers)
    - These interface description languages (IDLs) necessitate their own compilers, respectively `protoc` and `flatc`, which compile .proto or .fbs schemas into files for a specified programming language
- [JSON-RPC](https://en.wikipedia.org/wiki/JSON-RPC) is another option that doesn't use a proprietary serialization format

Furthermore, transmitted data can be compressed and decompressed using libraries such as `zlib`, `libbz2`, or `liblzma`. These libraries are used in the command-line tools `gzip`, `bzip2`, and `xz`, respectively.
