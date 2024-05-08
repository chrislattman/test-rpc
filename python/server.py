import os
import signal
import sys

import rpyc
from rpyc.utils.server import ThreadedServer

PORT_NUMBER = 5000
server: ThreadedServer


class Server(rpyc.Service):
    def exposed_open(self, path: str, oflag: int, *args) -> int:
        if oflag == os.O_CREAT:
            mode = args[0]
            fildes = os.open(path, oflag, mode)
        else:
            fildes = os.open(path, oflag)
        return fildes

    def exposed_close(self, fildes: int) -> None:
        os.close(fildes)

    def exposed_read(self, fildes: int, nbyte: int) -> bytes:
        return os.read(fildes, nbyte)

    def exposed_write(self, fildes: int, buf: bytes) -> int:
        return os.write(fildes, buf)

    def exposed_lseek(self, fildes: int, offset: int, whence: int) -> int:
        return os.lseek(fildes, offset, whence)

    def exposed_truncate(self, path: str, length: int) -> None:
        os.truncate(path, length)

    def exposed_ftruncate(self, fildes: int, length: int) -> None:
        os.ftruncate(fildes, length)

    def exposed_stat(self, path: str) -> os.stat_result:
        return os.stat(path)

    def exposed_fstat(self, fildes: int) -> os.stat_result:
        return os.fstat(fildes)

    def exposed_fsync(self, fildes: int) -> None:
        os.fsync(fildes)

    def exposed_rename(self, old: str, new: str) -> None:
        os.rename(old, new)

    def exposed_unlink(self, path: str) -> None:
        os.unlink(path)


def signal_handler(signum, frame) -> None:
    server.close()
    sys.exit(0)


def main():
    port_string = os.getenv("RPC_PORT")
    if port_string is not None:
        port_number = int(port_string)
    else:
        port_number = PORT_NUMBER
    signal.signal(signal.SIGINT, signal_handler)
    global server
    # To use a Unix domain socket (local socket):
    # server = ThreadedServer(Server, socket_path="/tmp/domain.sock")
    # In general, Unix domain sockets are constructed like IP sockets
    server = ThreadedServer(Server, port=port_number)
    server.start()


if __name__ == "__main__":
    main()
