package javarpc;

import java.rmi.registry.Registry;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.rmi.RemoteException;
import java.rmi.registry.LocateRegistry;
import java.rmi.server.UnicastRemoteObject;
import java.util.Arrays;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Main server loop for RPC server.
 */
public class Server implements RPCStubs {
    private static final int PORT_NUMBER = 1099;
    // RMI can run calls from different clients on different threads at the
    // same time, so the map must be thread-safe
    private final ConcurrentHashMap<Integer, RandomAccessFile> files;
    // hashCode() isn't guaranteed to be unique, so two open files could get
    // the same descriptor; hand out descriptors from a counter instead
    private final AtomicInteger nextFd;

    public Server() {
        files = new ConcurrentHashMap<>();
        nextFd = new AtomicInteger(3);
    }

    /**
     * Looks up an open file by descriptor.
     *
     * @param fd file descriptor
     * @return the open file
     * @throws IOException if fd isn't an open file descriptor
     */
    private RandomAccessFile getFile(int fd) throws IOException {
        RandomAccessFile file = files.get(fd);
        if (file == null) {
            throw new IOException("Bad file descriptor " + fd);
        }
        return file;
    }

    public static void main(String args[]) {
        String port = System.getenv("RPC_PORT");
        int portNumber = PORT_NUMBER;
        if (port != null) {
            portNumber = Integer.parseInt(port);
        }
        try {
            Server obj = new Server();
            // the port in exportObject can be ephemeral, what a client connects to is the rmiregistry port
            RPCStubs stub = (RPCStubs) UnicastRemoteObject.exportObject(obj, 0);
            // Unix domain sockets are not currently supported for RMI
            // In general, to setup a Unix domain socket server:
            // https://www.baeldung.com/java-unix-domain-socket#receivingMessages
            Registry registry = LocateRegistry.getRegistry(portNumber);
            registry.bind("RPCStubs", stub);
        } catch (Exception e) {
            System.err.println("Server exception: " + e.toString());
            e.printStackTrace();
        }
    }

    @Override
    public int open(String name, String mode) throws RemoteException, FileNotFoundException {
        RandomAccessFile file = new RandomAccessFile(name, mode);
        int fd = nextFd.getAndIncrement();
        files.put(fd, file);
        return fd;
    }

    @Override
    public void close(int fd) throws RemoteException, IOException {
        RandomAccessFile file = files.remove(fd);
        if (file == null) {
            throw new IOException("Bad file descriptor " + fd);
        }
        file.close();
    }

    @Override
    public byte[] read(int fd, int len) throws RemoteException, IOException {
        if (len < 0) {
            throw new IOException("Negative read length " + len);
        }
        RandomAccessFile file = getFile(fd);
        byte[] result = new byte[len];
        int status = file.read(result);
        if (status < 0) {
            result = null;
        } else if (status < len) {
            // Only return the bytes actually read, not the rest of the zeroed buffer
            result = Arrays.copyOf(result, status);
        }
        return result;
    }

    @Override
    public void write(int fd, byte[] b, int off, int len) throws RemoteException, IOException {
        RandomAccessFile file = getFile(fd);
        file.write(b, off, len);
    }

    @Override
    public void seek(int fd, long pos) throws RemoteException, IOException {
        RandomAccessFile file = getFile(fd);
        file.seek(pos);
    }

    @Override
    public void ftruncate(int fd, long length) throws RemoteException, IOException {
        RandomAccessFile file = getFile(fd);
        file.setLength(length);
    }

    @Override
    public long offset(int fd) throws RemoteException, IOException {
        RandomAccessFile file = getFile(fd);
        return file.getFilePointer();
    }

    @Override
    public long size(String name) throws RemoteException {
        File file = new File(name);
        return file.length();
    }

    @Override
    public long lastModifiedTime(String name) throws RemoteException {
        File file = new File(name);
        return file.lastModified();
    }

    @Override
    public void fsync(int fd) throws RemoteException, IOException {
        RandomAccessFile file = getFile(fd);
        file.getFD().sync();
    }

    @Override
    public boolean rename(String oldpath, String newpath) throws RemoteException {
        File oldFile = new File(oldpath);
        File newFile = new File(newpath);
        return oldFile.renameTo(newFile);
    }

    @Override
    public boolean unlink(String path) throws RemoteException {
        File fileToDelete = new File(path);
        return fileToDelete.delete();
    }
}
