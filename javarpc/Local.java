package javarpc;

import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.TimeZone;

public class Local {
    public static void main(String[] args) {
        File f = new File("test_file.txt");
        System.out.println("File size (in bytes): " + f.length());
        SimpleDateFormat sdf = new SimpleDateFormat("EEE, d MMM yyyy HH:mm:ss z");
        sdf.setTimeZone(TimeZone.getTimeZone("GMT"));
        System.out.println("Last modified time: " + sdf.format(new Date(f.lastModified())));

        try {
            RandomAccessFile file = new RandomAccessFile("test_file.txt", "rw");
            byte[] buf = new byte[200];
            int status = file.read(buf, 0, 5);
            System.out.println(status + " " + new String(buf, StandardCharsets.UTF_8));
            file.seek(0);
            status = file.read(buf, 5, 6);
            System.out.println(status + " " + new String(buf, StandardCharsets.UTF_8));
            file.write("word".getBytes(StandardCharsets.UTF_8), 0, 4);
            file.getFD().sync();
            file.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
