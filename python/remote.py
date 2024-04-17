import os
import time

import rpyc
import rpyc.utils.factory

HOSTNAME = "127.0.0.1"
PORT_NUMBER = 5000

host = os.getenv("RPC_HOST")
port = os.getenv("RPC_PORT")
if host is None:
    host = HOSTNAME
if port is None:
    port_number = PORT_NUMBER
else:
    port_number = int(port)

# To use a Unix domain socket (local socket):
# conn = rpyc.utils.factory.unix_connect("/tmp/domain.sock")
conn = rpyc.connect(host, port_number)

statbuf = conn.root.stat("test_file.txt")
statbuf = os.stat_result(statbuf)
print("File size (in bytes): " + str(statbuf.st_size))
print(
    "Last modified time: "
    + time.strftime("%a, %d %b %Y %X GMT", time.gmtime(int(statbuf.st_mtime)))
)

fd: int = conn.root.open("test_file.txt", os.O_RDWR)
buf: bytes = conn.root.read(fd, 5)
print(str(len(buf)) + " " + buf.decode())
conn.root.lseek(fd, 0, os.SEEK_SET)
buf2: bytes = conn.root.read(fd, 6)
print(str(len(buf2)) + " " + buf.decode() + buf2.decode())
conn.root.write(fd, b"word")
conn.root.fsync(fd)
conn.root.close(fd)
conn.close()
