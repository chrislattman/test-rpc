import java.nio.charset.StandardCharsets;
import java.rmi.registry.LocateRegistry;
import java.rmi.registry.Registry;

public class Remote {
    public static void main(String[] args) {
        String host = System.getenv("RPC_HOST");
        String port = System.getenv("RPC_PORT");
        int portNumber = 1099;
        if (port != null) {
            portNumber = Integer.parseInt(port);
        }
        try {
            Registry registry = LocateRegistry.getRegistry(host, portNumber);
            RPCStubs stub = (RPCStubs) registry.lookup("RPCStubs");

            int fd = stub.open("test_file.txt", "rw");
            byte[] buf = new byte[200];
            ReadResponse resp = stub.read(fd, 5);
            System.arraycopy(resp.b, 0, buf, 0, 5);
            System.out.println(resp.status + " " + new String(buf, StandardCharsets.UTF_8));
            stub.seek(fd, 0);
            resp = stub.read(fd, 6);
            System.arraycopy(resp.b, 0, buf, 5, 6);
            System.out.println(resp.status + " " + new String(buf, StandardCharsets.UTF_8));
            stub.write(fd, "word".getBytes(StandardCharsets.UTF_8), 0, 4);
            stub.close(fd);
        } catch (Exception e) {
            System.err.println("TestRemote exception: " + e.toString());
            e.printStackTrace();
        }
    }
}
