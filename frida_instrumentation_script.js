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
// Read this page for parsing C++ strings: https://learnfrida.info/intermediate_usage/#stdstring

// Detaches open library call listener once specific file is opened
let openListener = Interceptor.attach(
    Module.getExportByName(libc, "open"),
    {
        onEnter(args) {
            this.removeHook = false;
            const path = args[0].readUtf8String();
            if (path === "test_file.txt") {
                this.removeHook = true;
            }
            // Can also use JSON.stringify(obj) as first argument to send()
            send(`FRIDA: open path: ${path}`);
        },
        onLeave(retVal) {
            const fd = retVal.toInt32();
            send(`FRIDA: open return value: ${fd}`);
            if (this.removeHook) {
                send("FRIDA: removing instrumentation for 'open'");
                openListener.detach();
            }
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
            // this.originalPath = arg0;
            // const newPath = Memory.allocUtf8String("test_file1.txt");
            // this.newPath = newPath;
            if (path === "test_file.txt") {
                send(`FRIDA: stat path: ${path}`);
                statbuf = args[1];
            }
            // arg0 = newPath; // to modify the argument
        },
        onLeave(retVal) {
            // can access this.originalPath from here to see any changes made
            if (!ran) {
                let stSize;
                if (os === "darwin") {
                    stSize = statbuf.add(96).readS64();
                } else { // linux
                    stSize = statbuf.add(48).readS64();
                }
                const errCode = retVal.toInt32();
                send(`FRIDA: statbuf.st_size: ${stSize}`);
                send(`FRIDA: stat return value: ${errCode}`);
            }
            ran = true;
            // retVal.replace(ptr(1)); // to modify the return value
        }
    }
);
