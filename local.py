import os

fd = os.open("test_file.txt", os.O_RDWR)
buf = os.read(fd, 5)
print(str(len(buf)) + " " + buf.decode())
os.lseek(fd, 0, os.SEEK_SET)
buf2 = os.read(fd, 6)
print(str(len(buf2)) + " " + buf.decode() + buf2.decode())
status = os.write(fd, "word".encode())
os.close(fd)
