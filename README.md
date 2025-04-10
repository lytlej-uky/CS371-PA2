# CS371-PA2
Error control and flow control with UDP sockets

## Task 1
The goal of task 1 is to create a UDP-based "Stop-and-Wait" Protocol similar to PA1. Because UDP does not offer reliable transport, part of the task is to observe packet loss.

### A clean run without packet loss
To run without packet loss, try running the server `make run_server1` and running the client `make run_client1_no_drop`
You should observe that with a relatively small amount of clients, there is no packet loss

### With packet loss
To see the packet loss, you must try a larger number of clients. Try running the server `make run_server1` and running the client `make run_client1_drop` which will run with 1000 clients by default.
The number can be changed here by running the client like `make run_client1_drop NUM=<number of clients>`.

### Implementation details
This was accomplished by changing PA1's socket type to UDP as well as a few send and receive calls. Initially, we observed that with a large number of clients, the client side code would hang or get stuck after some time.
We realized this was because the packets were being lost and there was no mechanism to move on if there was a packet that was lost. So, we implemented the epoll timeout feature, and this allowed the program to move on after
a specified time because of a lost packet.

## Task 2
The goal of task 2 was to create a reliable transmission using Sequence Numbers and Automatic Repeat Request

### Running the reliable transmission test
To test the reliable transmission, you can run task 2 by executing `make run_server2` and `make run_client2 NUM=<number of clients>`. If you don't specify a number of clients, the default is 1000. We observed that this method
definitely required more time to transmit the data back and forth, but it was reliable to where the packets were sent in order.

### Implementation details
In order to implement this, we added a sequence number and client id that became part of the frame's header. To detect that a packet was lost, the client would have the same epoll timeout, but instead of moving on to the next
number, it would attempt to send it again until it received an acknowledge from the server. There is an additional check in the client side that detects if the ack number is the same as the sequence number it sent. 
That way if the data got damaged somehow, it could also try again.
