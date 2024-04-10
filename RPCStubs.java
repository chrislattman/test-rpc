import java.io.FileNotFoundException;
import java.io.IOException;
import java.rmi.Remote;
import java.rmi.RemoteException;

/**
 * Stub interface to expose method signatures to clients.
 */
public interface RPCStubs extends Remote {
    /**
     * Creates a random access file stream to read from, and optionally to write
     * to, a file with the specified name.
     * @param name the system-dependent filename
     * @param mode the access mode (can be either "r" or "rw")
     * @return the hash code to a RandomAccessFile object on the server
     * @throws RemoteException if the remote call fails
     * @throws FileNotFoundException if the file was open in read-only mode but
     * wasn't found or new file cannot be created
     */
    int open(String name, String mode) throws RemoteException, FileNotFoundException;

    /**
     * Closes this random access file stream and releases any system resources
     * associated with the stream.
     * @param fd hash code of the RandomAccessFile to close
     * @throws RemoteException if the remote call fails
     * @throws IOException if an I/O error occurs
     */
    void close(int fd) throws RemoteException, IOException;

    /**
     * Reads up to len bytes of data from this file into an array of bytes.
     * @param fd hash code of the RandomAccessFile to read from
     * @param len the maximum number of bytes read
     * @return a ReadResponse object which includes the buffer and the status,
     * which is the total number of bytes read into the buffer, or -1 if there
     * is no more data because the end of this file has been reached
     * @throws RemoteException if the remote call fails
     * @throws IOException if the first byte cannot be read for any reason other
     * than end of file, or if the random access file has been closed, or if
     * some other I/O error occurs
     */
    ReadResponse read(int fd, int len) throws RemoteException, IOException;

    /**
     * Writes b.length bytes from the specified byte array to this file,
     * starting at the current file pointer.
     * @param fd hash code of the RandomAccessFile to write to
     * @param b the buffer from which data is read from
     * @param off the start offset in array b
     * @param len the number of bytes to write
     * @throws RemoteException if the remote call fails
     * @throws IOException if an I/O error occurs
     */
    void write(int fd, byte[] b, int off, int len) throws RemoteException, IOException;

    /**
     * Sets the file-pointer offset, measured from the beginning of this file,
     * at which the next read or write occurs.
     * Use offset() and size() to move the file-pointer offset to a relative
     * location in the file.
     * @param fd hash code of the RandomAccessFile
     * @param pos the offset position, measured in bytes from the beginning of
     * the file, at which to set the file pointer
     * @throws RemoteException if the remote call fails
     * @throws IOException if pos is less than 0 or if an I/O error occurs
     */
    void seek(int fd, long pos) throws RemoteException, IOException;

    /**
     * Returns the current offset in this file.
     * @param fd hash code of the RandomAccessFile
     * @return the offset from the beginning of the file, in bytes, at which the
     * next read or write occurs
     * @throws RemoteException if the remote call fails
     * @throws IOException if an I/O error occurs
     */
    long offset(int fd) throws RemoteException, IOException;

    /**
     * Returns the length of this file.
     * @param fd hash code of the RandomAccessFile
     * @return the length of this file, measured in bytes
     * @throws RemoteException if the remote call fails
     * @throws IOException if an I/O error occurs
     */
    long size(int fd) throws RemoteException, IOException;

    // No native methods comparable to stat/fstat for RandomAccessFile objects
}
