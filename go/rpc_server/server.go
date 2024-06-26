package main

import (
	"example/rpc_types"
	"io/fs"
	"log"
	"net"
	"net/rpc"
	"os"
)

const PORT_NUMBER string = "5000"

type Receiver int

var files map[uintptr]*os.File

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
	files[file.Fd()] = file
	*reply = file.Fd()
	return nil
}

func (t *Receiver) Close(args *rpc_types.CloseArgs, reply *int) error {
	fildes := args.Fildes
	file := files[fildes]
	err := file.Close()
	delete(files, fildes)
	return err
}

func (t *Receiver) Read(args *rpc_types.ReadArgs, reply *rpc_types.ReadRet) error {
	fildes := args.Fildes
	nbyte := args.Nbyte
	file := files[fildes]
	reply.Buf = make([]byte, nbyte)
	_, err := file.Read(reply.Buf)
	return err
}

func (t *Receiver) Write(args *rpc_types.WriteArgs, reply *int) error {
	fildes := args.Fildes
	buf := args.Buf
	file := files[fildes]
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
	file := files[fildes]
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
	file := files[fildes]
	err := file.Truncate(length)
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
	file := files[fildes]
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
