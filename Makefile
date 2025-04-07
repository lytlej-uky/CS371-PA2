NUM?=100

run_server1: pa2_1
	./pa2 server 127.0.0.1 12345

run_client1: pa2_1
	./pa2 client 127.0.0.1 12345 4 10000

run_client_many1: pa2_1
	./pa2 client 127.0.0.1 12345 $(NUM) 1000

pa2_1: pa2_task1.c
	gcc -o pa2 pa2_task1.c -pthread

clean:
	rm -f pa2