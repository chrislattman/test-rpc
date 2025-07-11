# RPC client and server

Use the provided Makefile to test the [RPC](https://en.wikipedia.org/wiki/Remote_procedure_call) server and client.

This example leverages the `LD_PRELOAD` environment variable on Linux to hook the standard C library functions `open`, `close`, `read`, `write`, `lseek`, `truncate`, `ftruncate`, `stat`, `fstat`, `fsync`, `rename`, and `unlink` using a [shim](https://en.wikipedia.org/wiki/Shim_(computing)) (rpc_hooks.c). When a filename starts with "//", the function (`open`, `truncate`, `stat`, `rename`, or `unlink`) arguments are marshaled/serialized and sent via TCP to a remote RPC server to be unmarshaled/deserialized and executed there. Very large file descriptors (> 1,073,741,823) are assumed to have originated remotely, and functions that are passed in such file descriptors are likewise executed remotely, with their function arguments marshaled/serialized before transmission. Results from each of the function calls are marshaled/serialized in the server and sent back to the client via TCP, where they are subsequently unmarshaled/deserialized.

- On macOS, the `DYLD_INSERT_LIBRARIES` environment variable serves the same purpose as `LD_PRELOAD` on Linux; however, the target executable needs to be compiled using Apple clang with the flag `-flat_namespace` (this is due to `DYLD_FORCE_FLAT_NAMESPACE=1` no longer having any effect)
    - An alternative method for macOS is outlined in [`shim.c`](c/shim.c) (hooking the `puts` function)
    - A similar shim for the C++ `<<` operator for `std::ostream` can't be made due to C++ Standard Library restrictions

- On Windows, there is no `LD_PRELOAD` analog. However, Windows gives you the ability to allocate memory to another process, as well as write to that memory and run code in it. To achieve similar functionality as `LD_PRELOAD` on Windows, you have to hook the Import Address Table.
    - This is outlined in [`winshim.c`](c/winshim.c), which is injected by a separate file [`dll_injector.c`](c/dll_injector.c)
    - This process can be partially replicated on Linux by hooking the Global Offset Table, or on macOS by hooking the Lazy Symbol Pointer table
    - However, there is no similar API in Linux or macOS that allows you to allocate memory to another process
    - Using the `LD_PRELOAD`/`DYLD_INSERT_LIBRARIES` approach is more reliable (and more straightforward) on those platforms

Run `export RPC_HOST=<ip-address>` and/or `export RPC_PORT=<port-number>` as environment variables to specify custom values before running a remote test.

These function calls are all synchronous. With that said, there is varied support for asynchronous (async) file I/O in the major programming languages:

- Java has the [`AsynchronousFileChannel`](https://docs.oracle.com/en/java/javase/21/docs/api/java.base/java/nio/channels/AsynchronousFileChannel.html) class
- Python doesn't have native async file I/O support, but there are the [`aiofile`](https://pypi.org/project/aiofile/) and [`aiofiles`](https://pypi.org/project/aiofiles/) packages
- [Node.js](https://nodejs.org/api/fs.html) has two different APIs that support async file I/O: the original callback API and the newer Promises API
- Rust doesn't have native async file I/O support, but the popular [tokio](https://docs.rs/tokio/latest/tokio/io/index.html) crate offers it
- C# offers [asynchronous file I/O](https://learn.microsoft.com/en-us/dotnet/standard/io/asynchronous-file-i-o)
- C:
    - POSIX C has [`aio.h`](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/aio.h.html)
    - Win32 C allows you to pass the `FILE_FLAG_OVERLAPPED` flag to [`CreateFileA`](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea#synchronous_and_asynchronous_i_o_handles), which requires an `OVERLAPPED` structure to be allocated by the application and its pointer passed in to `ReadFile` and `WriteFile`

These examples all communicate with servers written in the same language. Well-known language-agnostic RPC frameworks include [gRPC](https://en.wikipedia.org/wiki/GRPC) and [Cap'n Proto](https://en.wikipedia.org/wiki/Cap%27n_Proto).

- These frameworks use their own data serialization formats
    - gRPC uses [Protocol Buffers (Protobuf)](https://en.wikipedia.org/wiki/Protocol_Buffers)
    - Another format is [FlatBuffers](https://en.wikipedia.org/wiki/FlatBuffers)
    - These interface description languages (IDLs) necessitate their own compilers, respectively `protoc` and `flatc`, which compile .proto or .fbs schemas into files for a specified programming language
- [JSON-RPC](https://en.wikipedia.org/wiki/JSON-RPC) is another option that doesn't use a proprietary serialization format

Furthermore, transmitted data can be compressed and decompressed using libraries such as `zlib`, `libbz2`, or `liblzma`. These libraries are used in the command-line tools `gzip`, `bzip2`, and `xz`, respectively.
