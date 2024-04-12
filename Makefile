CFLAGS=-Wall -Wextra -std=c99

test_local: rpc_stubs local
	LD_PRELOAD=./librpc.so ./local

test_local_java: rpc_stubs_java local_java
	java Local

test_local_python:
	python3 local.py

test_local_go:
	cd go; go run local_client/local.go

test_remote: rpc_stubs remote test_server
	LD_PRELOAD=./librpc.so ./remote

test_remote_java: rpc_stubs_java remote_java test_server_java
	java Remote

test_remote_python: test_server_python
	python3 remote.py

test_remote_go: test_server_go
	cd go; go run remote_client/remote.go

test_server: server
	./server &

test_server_java: server_java
	rmiregistry $(RPC_PORT) &
	sleep 1
	java -Djava.rmi.server.codebase=file:. Server &
	sleep 1

test_server_python:
	python3 server.py &

test_server_go:
	cd go; go run rpc_server/server.go &
	sleep 1

server:
	gcc $(CFLAGS) -pedantic -o server server.c

server_java: rpc_stubs_java
	javac Server.java

rpc_stubs:
	gcc $(CFLAGS) -shared -fpic -o librpc.so rpc_stubs.c

rpc_stubs_java:
	javac RPCStubs.java

local:
	gcc $(CFLAGS) -pedantic -o local local.c

local_java:
	javac Local.java

remote:
	gcc $(CFLAGS) -pedantic -o remote remote.c

remote_java:
	javac Remote.java

clean:
	rm -f server local remote *.so *.class
	pkill -9 server || true
	pkill -9 rmiregistry || true
	pkill -9 java || true
	pkill -9 python3 || true
	pkill -9 go || true
