1.	File Description:
Readme.txt: This file.
Readme.docx: A word version of this file. The contents are the same.
client.c & server.c: Source code
client.out & server.out: Complied file
input.txt: a sample input
Example (Folder): An example of one server (CBP) and two clients (SP). You can find the output files in each directories.
2.	Compilation & Execute:
Server and clients are supposed to be put in different directories (as shown in folder ¡°example¡±), for the output file has the same name.
To compile and execute the project, following commands are supposed to be used:
Server:
cc server.c -o server.out
./server.out
Clients:
cc client.c -o client.out 
./client.out

3.	Input
Server:
After executing the ./server.out, port number and number of clients should be input as shown by following lines:
Enter port number:
Enter number of Station processes (up to 10)
Port number could be any number. A number larger than 1024 is recommended.
Number of station processes is recommended at the maximum of 9 for the maximum number of connection is 10.
Clients:
After executing ./client.out, several information of the client should be input as shown by following lines:
Enter Client ID: 
Enter SERVER's IP:
Enter SERVER's port number: 
Client ID should be 1-9 for we set 10 as maximum number of connection.
Wait time should be 1-10 as a level. 1 is shorter and 10 is longer.
Noted that port number and IP are supposed to be the SERVER¡¯s, not the clients¡¯.

The program will try to connect the client and the server with given information. If the connection is failed, a related error information will be shown.
Frame information
The frame information is simple as the frame number and the destination. An example input is given in input.txt as test data.
Every client should has its own input.txt file in the same directory as its client.c file.

4.	Output
The running information of server and clients are saved in output.txt. You can see all collision occurred in it.
Server and clients are supposed to be put in different directories, for the output file has the same name.

5.	Project Features
Station Process
Station Process is implemented in client.c
After a client is set and connected to the server, it will begin sending frames as given in input.txt. The client ID can be seen on server¡¯s process as long as it is connected. The output information, as the success of frame sending or 
Communication Bus Process
Communication bus process is implemented in server.c.
Server will be put in listening states, waiting for all clients are connected (client number is given when start up server). After that, a message will be shown and the server are ready to receive frames sent by clients.
Collision
As the program detected a collision, an integer variable as a flag will be set as a counter of collision happened to this client. The initial counter is set as 0. If the number of collision happened is less than 10, the time slot will be a random number of 2pow number of collision. After 10 collisions, the up bound is 2pow10 (1024). Process will terminate after 16 collisions.
