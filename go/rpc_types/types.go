package rpc_types

import (
	"io/fs"
)

type OpenArgs struct {
	Path  string
	Oflag int
	Mode  fs.FileMode
}

type CloseArgs struct {
	Fildes uintptr
}

type ReadArgs struct {
	Fildes uintptr
	Nbyte  int
}

type ReadRet struct {
	Buf []byte
}

type WriteArgs struct {
	Fildes uintptr
	Buf    []byte
}

type LseekArgs struct {
	Fildes uintptr
	Offset int64
	Whence int
}

type SizeArgs struct {
	Path string
}

type LastModifiedTimeArgs struct {
	Path string
}

type FsyncArgs struct {
	Fildes uintptr
}
