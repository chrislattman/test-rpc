package main

import (
	"example/rpc_types"
	"fmt"
	"io"
	"log"
	"net/rpc"
	"os"
	"strings"
	"time"
)

const HOSTNAME string = "127.0.0.1"
const PORT_NUMBER string = "5000"

func main() {
	host := os.Getenv("RPC_HOST")
	port := os.Getenv("RPC_PORT")
	if len(host) == 0 {
		host = HOSTNAME
	}
	if len(port) == 0 {
		port = PORT_NUMBER
	}
	client, err := rpc.Dial("tcp", host + ":" + port)
	if err != nil {
		log.Fatal("rpc.Dial:", err)
	}

	size_args := &rpc_types.SizeArgs{Path: "../test_file.txt"}
	var size_reply int64
	err = client.Call("Receiver.Size", size_args, &size_reply)
	if err != nil {
		log.Fatal("rpc.Client.Call:", err)
	}
	fmt.Println("File size (in bytes):", size_reply)
	lastmodifiedtime_args := &rpc_types.LastModifiedTimeArgs{Path: "../test_file.txt"}
	var lastmodifiedtime_reply int64
	err = client.Call("Receiver.LastModifiedTime", lastmodifiedtime_args, &lastmodifiedtime_reply)
	if err != nil {
		log.Fatal("rpc.Client.Call:", err)
	}
	date := time.Unix(lastmodifiedtime_reply, 0).UTC().Format(time.RFC1123)
	fmt.Println("Last modified time:", strings.Replace(date, "UTC", "GMT", 1))

	open_args := &rpc_types.OpenArgs{Path: "../test_file.txt", Oflag: os.O_RDWR, Mode: 0644}
	var fd uintptr
	err = client.Call("Receiver.Open", open_args, &fd)
	if err != nil {
		log.Fatal("rpc.Client.Call:", err)
	}
	read_args := &rpc_types.ReadArgs{Fildes: fd, Nbyte: 5}
	var buf rpc_types.ReadRet
	err = client.Call("Receiver.Read", read_args, &buf)
	if err != nil {
		log.Fatal("rpc.Client.Call:", err)
	}
	fmt.Println(len(buf.Buf), string(buf.Buf))
	lseek_args := &rpc_types.LseekArgs{Fildes: fd, Offset: 0, Whence: io.SeekStart}
	err = client.Call("Receiver.Lseek", lseek_args, nil)
	if err != nil {
		log.Fatal("rpc.Client.Call:", err)
	}
	read_args = &rpc_types.ReadArgs{Fildes: fd, Nbyte: 6}
	var buf2 rpc_types.ReadRet
	err = client.Call("Receiver.Read", read_args, &buf2)
	if err != nil {
		log.Fatal("rpc.Client.Call:", err)
	}
	fmt.Println(len(buf2.Buf), string(buf.Buf) + string(buf2.Buf))
	write_args := &rpc_types.WriteArgs{Fildes: fd, Buf: []byte("word")}
	err = client.Call("Receiver.Write", write_args, nil)
	if err != nil {
		log.Fatal("rpc.Client.Call:", err)
	}
	sync_args := &rpc_types.FsyncArgs{Fildes: fd}
	err = client.Call("Receiver.Fsync", sync_args, nil)
	if err != nil {
		log.Fatal("rpc.Client.Call:", err)
	}
	close_args := &rpc_types.CloseArgs{Fildes: fd}
	err = client.Call("Receiver.Close", close_args, nil)
	if err != nil {
		log.Fatal("rpc.Client.Call:", err)
	}
	client.Close()
}
