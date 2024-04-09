CFLAGS=-Wall -Wextra -std=c99

local: client_stubs test_local
	LD_PRELOAD=./librpc.so ./test_local

remote: server client_stubs test_remote
	LD_PRELOAD=./librpc.so ./test_remote

server:
	gcc $(CFLAGS) -pedantic -o server server.c

client_stubs:
	gcc $(CFLAGS) -shared -fpic -o librpc.so rpc_stubs.c

test_local:
	gcc $(CFLAGS) -pedantic -o test_local test_local.c

test_remote:
	gcc $(CFLAGS) -pedantic -o test_remote test_remote.c

clean:
	rm -f server test_local test_remote librpc.so
