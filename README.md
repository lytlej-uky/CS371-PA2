# CS371-PA2
Error control and flow control with UDP sockets

## Compiling the project
`gcc -o pa2 pa2.c -pthread`

## Example Run
Start the server first with designated IP and port number

`./pa2 server 127.0.0.1 12345`

Then run the client with 4 threads, each thread sends 1000000 messages.

The client threads will connect with designated server IP and port number

`./pa2 client 127.0.0.1 12345 4 1000000`
