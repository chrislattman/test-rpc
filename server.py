import os
import rpyc
from rpyc.utils.server import ThreadedServer

PORT_NUMBER = 5000


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

    def exposed_stat(self, path: str) -> os.stat_result:
        return os.stat(path)

    def exposed_fstat(self, fildes: int) -> os.stat_result:
        return os.fstat(fildes)


if __name__ == "__main__":
    port_string = os.getenv("RPC_PORT")
    if port_string is not None:
        port_number = int(port_string)
    else:
        port_number = PORT_NUMBER
    t = ThreadedServer(Server, port=port_number)
    t.start()
