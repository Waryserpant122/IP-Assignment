Roll no 106118107
Yash Shah
CSE A

This is a DHCP Simulation done in c++ using threads,pthread of the Linux OS.

To run the code use 
g++ -pthread codeDHCP.cpp -o code
./code

The screenshots of the terminal have been included in the folder named Screenshots

The whole text of the terminal output has been copied and pasted in a file
named outputScreenshots.txt

For the simulation here I am using 3 servers running parallely on the serverFunc
and 5 clients running on the thread clientFunc.

The server are named server1,server2,server3;
Clients are named client1,client2,client3,client4,client5;

Here client 4 and 5 have no IP address assigned to them initially and ask the network for 
Ip addresses

And client 1 need to renew his IP address.
