run_server: pa2
	./pa2 server 127.0.0.1 12345
run_client: pa2	
	./pa2 client 127.0.0.1 12345 4 10000
pa2:
	gcc -o pa2 pa2.c -pthread

clean:
	rm -f pa2