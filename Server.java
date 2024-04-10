import java.rmi.registry.Registry;
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
    private HashMap<Integer, RandomAccessFile> files;

    public Server() {
        files = new HashMap<>();
    }

    public static void main(String args[]) {
        try {
            Server obj = new Server();
            // the port in exportObject can be ephemeral, what a client connects to is the rmiregistry port
            RPCStubs stub = (RPCStubs) UnicastRemoteObject.exportObject(obj, 0);
            Registry registry = LocateRegistry.getRegistry();
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
    public ReadResponse read(int fd, int len) throws RemoteException, IOException {
        RandomAccessFile file = files.get(fd);
        ReadResponse resp = new ReadResponse();
        resp.b = new byte[len];
        resp.status = file.read(resp.b);
        return resp;
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
    public long size(int fd) throws RemoteException, IOException {
        RandomAccessFile file = files.get(fd);
        return file.length();
    }
}
