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
stub = conn.root

statbuf = stub.stat("test_file.txt")
statbuf = os.stat_result(statbuf)
print("File size (in bytes): " + str(statbuf.st_size))
print(
    "Last modified time: "
    + time.strftime("%a, %d %b %Y %X GMT", time.gmtime(int(statbuf.st_mtime)))
)

fd: int = stub.open("test_file.txt", os.O_RDWR)
buf: bytes = stub.read(fd, 5)
print(str(len(buf)) + " " + buf.decode())
stub.lseek(fd, 0, os.SEEK_SET)
buf2: bytes = stub.read(fd, 6)
print(str(len(buf2)) + " " + buf.decode() + buf2.decode())
stub.write(fd, b"word")
stub.fsync(fd)
stub.close(fd)

stub.rename("test_file.txt", "renamed_file.txt")
stub.unlink("deleted_file.txt")
conn.close()
