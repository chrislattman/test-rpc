import java.nio.charset.StandardCharsets;
import java.rmi.registry.LocateRegistry;
import java.rmi.registry.Registry;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.TimeZone;

public class Remote {
    private static final int PORT_NUMBER = 1099;

    public static void main(String[] args) {
        String host = System.getenv("RPC_HOST");
        String port = System.getenv("RPC_PORT");
        int portNumber = PORT_NUMBER;
        if (port != null) {
            portNumber = Integer.parseInt(port);
        }
        try {
            Registry registry = LocateRegistry.getRegistry(host, portNumber);
            RPCStubs stub = (RPCStubs) registry.lookup("RPCStubs");

            System.out.println("File size (in bytes): " + stub.size("test_file.txt"));
            SimpleDateFormat sdf = new SimpleDateFormat("EEE, d MMM yyyy HH:mm:ss z");
            sdf.setTimeZone(TimeZone.getTimeZone("GMT"));
            System.out.println("Last modified time: " + sdf.format(
                new Date(stub.lastModifiedTime("test_file.txt"))));
            int fd = stub.open("test_file.txt", "rw");
            byte[] buf = new byte[200];
            byte[] resp = stub.read(fd, 5);
            System.arraycopy(resp, 0, buf, 0, 5);
            System.out.println(resp.length + " " + new String(buf, StandardCharsets.UTF_8));
            stub.seek(fd, 0);
            byte[] resp2 = stub.read(fd, 6);
            System.arraycopy(resp2, 0, buf, 5, 6);
            System.out.println(resp2.length + " " + new String(buf, StandardCharsets.UTF_8));
            stub.write(fd, "word".getBytes(StandardCharsets.UTF_8), 0, 4);
            stub.close(fd);
        } catch (Exception e) {
            System.err.println("TestRemote exception: " + e.toString());
            e.printStackTrace();
        }
    }
}
