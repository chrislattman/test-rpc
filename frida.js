const os = Process.platform.toString();
let libc;
if (os === "darwin") {
    libc = "libSystem.B.dylib";
} else if (os === "linux") {
    libc = "libc.so.6";
} else {
    throw new Error(`Unsupported platform: ${os}`);
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

// if using in multiple functions (no need for this.newPath):
// const newPath = Memory.allocUtf8String("test_file1.txt");
let statbuf = ptr(0);
let ran = false;
Interceptor.attach(
    Module.getExportByName(libc, "stat64"),
    {
        onEnter(args) {
            const arg0 = args[0];
            const path = arg0.readUtf8String();
            // const newPath = Memory.allocUtf8String("test_file1.txt");
            // this.newPath = newPath;
            if (path === "test_file.txt") {
                console.log(`FRIDA: stat path: ${path}`);
                statbuf = args[1];
            }
            // arg0 = newPath; // to modify the argument
        },
        onLeave: (retVal) => {
            if (!ran) {
                let stSize;
                if (os === "darwin") {
                    stSize = statbuf.add(96).readS64();
                } else { // linux
                    stSize = statbuf.add(48).readS64();
                }
                const errCode = retVal.toInt32();
                console.log(`FRIDA: statbuf.st_size: ${stSize}`);
                console.log(`FRIDA: stat return value: ${errCode}`);
            }
            ran = true;
            // retVal.replace(ptr(1)); // to modify the return value
        }
    }
);
