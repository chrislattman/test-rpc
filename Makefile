CFLAGS=-Wall -Wextra -std=c99

test_local: rpc_stubs local
	LD_PRELOAD=./librpc.so ./local

test_local_frida: local
	frida -f ./local -l frida.js || true

test_local_frida_python: local
	python3 frida_control_script.py

test_local_win:
	gcc -Wall -Wextra -std=c99 -pedantic -o local c/winlocal.c
	./local

test_local_java: rpc_stubs_java local_java
	java javarpc.Local

test_local_python:
	python3 python/local.py

test_local_go:
	cd go; go run local_client/local.go

test_local_js:
	node nodejs/local.js

test_local_rust:
	cargo run -q --bin local

test_remote: rpc_stubs remote test_server
	LD_PRELOAD=./librpc.so ./remote

test_remote_java: rpc_stubs_java remote_java test_server_java
	java javarpc.Remote

test_remote_python: test_server_python
	python3 python/remote.py

test_remote_go: test_server_go
	cd go; go run remote_client/remote.go

test_server: server
	./server &

test_server_java: server_java
	rmiregistry $(RPC_PORT) &
	sleep 1
	java -Djava.rmi.server.codebase=file:. javarpc.Server &
	sleep 1

test_server_python:
	python3 python/server.py &

test_server_go:
	cd go; go run rpc_server/server.go &
	sleep 1

server:
	gcc $(CFLAGS) -pedantic -o server c/server.c

server_java: rpc_stubs_java
	javac javarpc/Server.java

rpc_stubs:
	gcc $(CFLAGS) -shared -fpic -o librpc.so c/rpc_stubs.c

rpc_stubs_java:
	javac javarpc/RPCStubs.java

local:
	gcc $(CFLAGS) -pedantic -o local c/local.c

local_java:
	javac javarpc/Local.java

remote:
	gcc $(CFLAGS) -pedantic -o remote c/remote.c

remote_java:
	javac javarpc/Remote.java

clean:
	rm -rf server local remote *.so javarpc/*.class target renamed_file.txt
	echo "Hello this is just some random text.\n1 2 3 4 5" > test_file.txt
	touch deleted_file.txt
	pkill -9 server || true
	pkill -9 rmiregistry || true
	pkill -9 java || true
	pkill -9 python3 || true
	pkill -9 go || true
