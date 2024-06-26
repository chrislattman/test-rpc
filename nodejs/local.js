const fs = require("node:fs");

const stat = fs.statSync("test_file.txt");
console.log("File size (in bytes): " + stat.size);
console.log("Last modified time: " + stat.mtime.toUTCString());

const fd = fs.openSync("test_file.txt", "rs+");
const buf = Buffer.alloc(200);
let status = fs.readSync(fd, buf, 0, 5);
console.log(status + " " + buf);
// Adding the 5th argument to readSync is equivalent to calling
// lseek(fd, 0, SEEK_SET) beforehand
status = fs.readSync(fd, buf, 5, 6, 0);
console.log(status + " " + buf);
fs.writeSync(fd, " word"); // need leading space for some reason
fs.ftruncateSync(fd, 30);
fs.fsyncSync(fd);
fs.closeSync(fd);

fs.renameSync("test_file.txt", "renamed_file.txt");
fs.unlinkSync("deleted_file.txt");

fs.readdirSync(".", {withFileTypes: true}).forEach((entry) => {
    if (entry.isDirectory()) {
        console.log(entry.name + "/");
    }
    if (entry.isFile()) {
        console.log(entry.name);
    }
});

// To use Unix domain sockets:
// Server: https://nodejs.org/api/net.html#serverlistenpath-backlog-callback
// Client: https://nodejs.org/api/net.html#netcreateconnectionpath-connectlistener
