NUM?=1000

######################################
# Task 1
######################################

# Run the server
run_server1: pa2_1
	./pa2_1 server 127.0.0.1 12345

# Run the client with no dropped frames
run_client1_no_drop: pa2_1
	./pa2_1 client 127.0.0.1 12345 100 1000

# Run the client for with dropped frames 
run_client1_drop: pa2_1
	./pa2_1 client 127.0.0.1 12345 $(NUM) 1000

# Compile the server/client code
pa2_1: pa2_task1.c
	gcc -o pa2_1 pa2_task1.c -pthread


######################################
# Task 2
######################################

# Run the server
run_server2: pa2_2
	./pa2_2 server 127.0.0.1 12345

# Run the client with NUM=<number of clients> and no dropped frames
run_client2: pa2_2
	./pa2_2 client 127.0.0.1 12345 $(NUM) 1000

# Compile the server/client code
pa2_2: pa2_task2.c
	gcc -g -o pa2_2 pa2_task2.c -pthread

# remove the build files
clean:
	rm -f pa2_1 pa2_2