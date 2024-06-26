import os
import time

statbuf = os.stat("test_file.txt")
print("File size (in bytes): " + str(statbuf.st_size))
print(
    "Last modified time: "
    + time.strftime("%a, %d %b %Y %X GMT", time.gmtime(int(statbuf.st_mtime)))
)

fd = os.open("test_file.txt", os.O_RDWR)
buf = os.read(fd, 5)
print(str(len(buf)) + " " + buf.decode())
os.lseek(fd, 0, os.SEEK_SET)
buf2 = os.read(fd, 6)
print(str(len(buf2)) + " " + buf.decode() + buf2.decode())
os.write(fd, b"word")
os.ftruncate(fd, 30)
os.fsync(fd)
os.close(fd)

os.rename("test_file.txt", "renamed_file.txt")
os.unlink("deleted_file.txt")

namelist = os.scandir(".")
for entry in namelist:
    if entry.is_dir():
        print(entry.name + "/")
    if entry.is_file():
        print(entry.name)
