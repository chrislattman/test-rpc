import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.charset.StandardCharsets;

public class Local {
    public static void main(String[] args) {
        try {
            RandomAccessFile file = new RandomAccessFile("test_file.txt", "rw");
            byte[] buf = new byte[200];
            int status = file.read(buf, 0, 5);
            System.out.println(status + " " + new String(buf, StandardCharsets.UTF_8));
            file.seek(0);
            status = file.read(buf, 5, 6);
            System.out.println(status + " " + new String(buf, StandardCharsets.UTF_8));
            file.write("word".getBytes(StandardCharsets.UTF_8), 0, 4);
            file.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
