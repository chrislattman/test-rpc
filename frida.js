const os = Process.platform.toString();
let libc;
if (os === "darwin") {
    libc = "libSystem.B.dylib";
} else if (os === "linux") { // linux
    libc = "libc.so";
} else {
    throw new Error("Unsupported platform");
}
// Windows libc is msvcrt.dll but obviously not POSIX-compliant

Interceptor.attach(
    Module.getExportByName(libc, "open"),
    {
        onEnter: (args) => {
            const path = args[0].readUtf8String();
            console.log(`FRIDA: open path: ${path}`);
        },
        onLeave: (retVal) => {
            const fd = retVal.toInt32();
            console.log(`FRIDA: open return value: ${fd}`);
        }
    }
);

let statbuf = ptr(0);
let ran = false;
Interceptor.attach(
    Module.getExportByName(libc, "stat64"),
    {
        onEnter: (args) => {
            const path = args[0].readUtf8String();
            if (path === "test_file.txt") {
                console.log(`FRIDA: stat path: ${path}`);
                statbuf = args[1];
            }
            // args[0] = ptr("test_file1.txt"); // to modify the argument
        },
        onLeave: (retVal) => {
            if (!ran) {
                const stSize = statbuf.add(96).readS64();
                const errCode = retVal.toInt32();
                console.log(`FRIDA: statbuf.st_size: ${stSize}`);
                console.log(`FRIDA: stat return value: ${errCode}`);
            }
            ran = true;
            // retVal.replace(ptr(1)); // to modify the return value
        }
    }
);
