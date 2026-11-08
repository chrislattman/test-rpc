package main

import (
	"errors"
	"example/rpc_types"
	"fmt"
	"io"
	"io/fs"
	"log"
	"net"
	"net/rpc"
	"os"
	"sync"
)

const PORT_NUMBER string = "5000"

type Receiver int

// net/rpc runs each call in its own goroutine, so access to files must be
// synchronized (concurrent map writes crash the whole server)
var files map[uintptr]*os.File
var filesMutex sync.Mutex

// Looks up an open file by descriptor. Returns an error instead of a nil
// *os.File for unknown descriptors: net/rpc doesn't recover panics, so a nil
// pointer dereference from one bad call would crash the whole server.
func getFile(fildes uintptr) (*os.File, error) {
	filesMutex.Lock()
	defer filesMutex.Unlock()
	file, ok := files[fildes]
	if !ok {
		return nil, fmt.Errorf("bad file descriptor %d", fildes)
	}
	return file, nil
}

func (t *Receiver) Open(args *rpc_types.OpenArgs, reply *uintptr) error {
	path := args.Path
	oflag := args.Oflag
	var mode fs.FileMode
	if oflag == os.O_CREATE {
		mode = args.Mode
	} else {
		mode = 0644
	}
	file, err := os.OpenFile(path, oflag, mode)
	if err != nil {
		return err
	}
	filesMutex.Lock()
	files[file.Fd()] = file
	filesMutex.Unlock()
	*reply = file.Fd()
	return nil
}

func (t *Receiver) Close(args *rpc_types.CloseArgs, reply *int) error {
	fildes := args.Fildes
	filesMutex.Lock()
	file, ok := files[fildes]
	delete(files, fildes)
	filesMutex.Unlock()
	if !ok {
		return fmt.Errorf("bad file descriptor %d", fildes)
	}
	return file.Close()
}

func (t *Receiver) Read(args *rpc_types.ReadArgs, reply *rpc_types.ReadRet) error {
	fildes := args.Fildes
	nbyte := args.Nbyte
	// make() panics on a negative size
	if nbyte < 0 {
		return errors.New("negative read size")
	}
	file, err := getFile(fildes)
	if err != nil {
		return err
	}
	reply.Buf = make([]byte, nbyte)
	n, err := file.Read(reply.Buf)
	// Only return the bytes actually read, not the rest of the zeroed buffer
	reply.Buf = reply.Buf[:n]
	// Like read(2), reaching end of file just means 0 bytes were read
	if errors.Is(err, io.EOF) {
		return nil
	}
	return err
}

func (t *Receiver) Write(args *rpc_types.WriteArgs, reply *int) error {
	fildes := args.Fildes
	buf := args.Buf
	file, err := getFile(fildes)
	if err != nil {
		return err
	}
	nbyte, err := file.Write(buf)
	if err != nil {
		return err
	}
	*reply = nbyte
	return nil
}

func (t *Receiver) Lseek(args *rpc_types.LseekArgs, reply *int64) error {
	fildes := args.Fildes
	offset := args.Offset
	whence := args.Whence
	file, err := getFile(fildes)
	if err != nil {
		return err
	}
	resulting_offset, err := file.Seek(offset, whence)
	if err != nil {
		return err
	}
	*reply = resulting_offset
	return nil
}

func (t *Receiver) Truncate(args *rpc_types.TruncateArgs, reply *int64) error {
	path := args.Path
	length := args.Length
	err := os.Truncate(path, length)
	if err != nil {
		return err
	}
	return nil;
}

func (t *Receiver) Ftruncate(args *rpc_types.FtruncateArgs, reply *int64) error {
	fildes := args.Fildes
	length := args.Length
	file, err := getFile(fildes)
	if err != nil {
		return err
	}
	err = file.Truncate(length)
	if err != nil {
		return err
	}
	return nil;
}

func (t *Receiver) Size(args *rpc_types.SizeArgs, reply *int64) error {
	path := args.Path
	statbuf, err := os.Stat(path)
	if err != nil {
		return err
	}
	*reply = statbuf.Size()
	return nil
}

func (t *Receiver) LastModifiedTime(args *rpc_types.LastModifiedTimeArgs, reply *int64) error {
	path := args.Path
	statbuf, err := os.Stat(path)
	if err != nil {
		return err
	}
	*reply = statbuf.ModTime().Unix()
	return nil
}

func (t *Receiver) Fsync(args *rpc_types.FsyncArgs, reply *int) error {
	fildes := args.Fildes
	file, err := getFile(fildes)
	if err != nil {
		return err
	}
	return file.Sync()
}

func (t *Receiver) Rename(args *rpc_types.RenameArgs, reply *int) error {
	oldpath := args.Old
	newpath := args.New
	return os.Rename(oldpath, newpath)
}

func (t *Receiver) Unlink(args *rpc_types.UnlinkArgs, reply *int) error {
	path := args.Path
	return os.Remove(path)
}

func main() {
	port := os.Getenv("RPC_PORT")
	if len(port) == 0 {
		port = PORT_NUMBER
	}
	files = make(map[uintptr]*os.File)
	receiver := new(Receiver)
	rpc.Register(receiver)

	// To use a Unix domain socket (local socket):
	// ln, err := net.Listen("unix", "/tmp/domain.sock")
	ln, err := net.Listen("tcp", ":"+port)
	if err != nil {
		log.Fatal("listen error:", err)
	}
	for {
		rpc.Accept(ln)
	}
}
