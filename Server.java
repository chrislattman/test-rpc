import java.rmi.registry.Registry;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.rmi.RemoteException;
import java.rmi.registry.LocateRegistry;
import java.rmi.server.UnicastRemoteObject;
import java.util.HashMap;

/**
 * Main server loop for RPC server.
 */
public class Server implements RPCStubs {
    private static final int PORT_NUMBER = 1099;
    private HashMap<Integer, RandomAccessFile> files;

    public Server() {
        files = new HashMap<>();
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
        files.put(file.hashCode(), file);
        return file.hashCode();
    }

    @Override
    public void close(int fd) throws RemoteException, IOException {
        RandomAccessFile file = files.get(fd);
        file.close();
        files.remove(fd);
    }

    @Override
    public byte[] read(int fd, int len) throws RemoteException, IOException {
        RandomAccessFile file = files.get(fd);
        byte[] result = new byte[len];
        int status = file.read(result);
        if (status < 0) {
            result = null;
        }
        return result;
    }

    @Override
    public void write(int fd, byte[] b, int off, int len) throws RemoteException, IOException {
        RandomAccessFile file = files.get(fd);
        file.write(b, off, len);
    }

    @Override
    public void seek(int fd, long pos) throws RemoteException, IOException {
        RandomAccessFile file = files.get(fd);
        file.seek(pos);
    }

    @Override
    public long offset(int fd) throws RemoteException, IOException {
        RandomAccessFile file = files.get(fd);
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
        RandomAccessFile file = files.get(fd);
        file.getFD().sync();
    }
}
