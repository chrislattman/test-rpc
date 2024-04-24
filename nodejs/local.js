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
fs.fsyncSync(fd);
fs.closeSync(fd);

fs.renameSync("test_file.txt", "renamed_file.txt");
fs.unlinkSync("deleted_file.txt");
