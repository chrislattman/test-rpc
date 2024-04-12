package main

import (
	"fmt"
	"io"
	"os"
	"strings"
	"time"
)

func main() {
	statbuf, _ := os.Stat("../test_file.txt")
	fmt.Println("File size (in bytes):", statbuf.Size())
	date := statbuf.ModTime().UTC().Format(time.RFC1123)
	fmt.Println("Last modified time:", strings.Replace(date, "UTC", "GMT", 1))
	file, _ := os.OpenFile("../test_file.txt", os.O_RDWR, 0644)
	buf := make([]byte, 5)
	status, _ := file.Read(buf)
	fmt.Println(status, string(buf))
	file.Seek(0, io.SeekStart)
	buf2 := make([]byte, 6)
	status, _ = file.Read(buf2)
	fmt.Println(status, string(buf) + string(buf2))
	file.Write([]byte("word"))
	file.Close()
}
