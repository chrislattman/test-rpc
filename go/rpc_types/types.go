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

type TruncateArgs struct {
	Path   string
	Length int64
}

type FtruncateArgs struct {
	Fildes uintptr
	Length int64
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

type RenameArgs struct {
	Old string
	New string
}

type UnlinkArgs struct {
	Path string
}
