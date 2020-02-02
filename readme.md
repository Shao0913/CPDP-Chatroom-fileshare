Project 2: A Messenger Application

Author: Yuanhang Shao (ys18c@my.fsu.edu)

Achieved features:
1. The server only supports 30 clients connection at one time.

2. The client only supports 20 client to client sending message at one time.

3. You need to follow the input format in client presented below, otherwise you may got some error.

4. Make sure the "servhost" in "configration_file_c*" (client configration files) is the IP address get by "hostname -I".

5. Make sure the "port" in "configration_file_c*" is not used by other application.

6. Make sure the "port" in "configration_file_s1" is not used by other application.

7. Make sure the "port" in "configration_file_c*" is as same as it in "configration_file_s1".

8. Make sure the listen port (default 5520) for client is not used by other application (if used by other applications, it will should init fault or bind fault), other wise you have to change the code at `line 139, function messenger_client::messenger_client(const char* c), file ./src/messenger_client.cpp` -- change the `this->listen_port = 5520;` to the avaliable port number.

Functions:
1. Using make command to compile the project.
$make

2. Using ssh command to covert some of terminals to another server.
$ssh username@linprog1.cs.fsu.edu

3.Start run server and clients in different terminal with configration files.
$./server ./user_info_file ./configration_file_s1
$./client ./configration_file_c1

4.After connect you can follow the instruction in the project to use command.
4.1 register new account
$r
$username password

4.2 login
$l
$username password

4.3 exit
$exit

5. After login you can follow the instruction in the project to use command.
5.1 inviation new friend
$i username hello_msg

5.2accept new friend inviatation
$a username greating_msg

5.3 send message to friend
$m friend_name msg

5.4 log out current login user
$logout

6 The server terminal will show some information to review.

Bugs and Limitation:
1. There may some unknow bugs or some problem caused by package loss or connection.
2. There is instability for inviate friend. The specific case is when the first user is new register and send inviation to default user1. After user1 accept the inviation, the user1 will be able to send msg to new register, but it not always work when the new register send msg to user1, unless new register re-log.
Example demo steps: register new client1, login client1 and user1, send inviation from client1 to user1, user1 accept and send inviation back to client1, then logout client1 and re-login client1, then client1 can send msg to new friend user1.
3. (This bug fixed in second submission, please use second submission.) During the debug, the client that use the same machine that connect to server will fail to connect keep on login_loop by show "Line:  944. Bind socket failed." by using address "0.0.0.0".

Key requriment:
1. POSIX threads -- at the server
In file "messenger_server.h", function "void messenger_server::loop()", "line 258-294". When a client want to connect to the server, the server will accept this connection and create a new thread to handle the information received from this client.

2. select() system call -- at the client
In file "messenger_client.h", function "void messenger_client::login_loop()", "line 524-683". The client will add the "client to server socket" (this->c2s_socket), "listen client connection socket" (`this->listen_socket`), and "client to client socket" (`vector<int> c2c_socket_vc`) to "FD_SET". Each new require to connect the client from other client, will added to the "vector<int> c2c_socket_vc" and selecting and then check if it sends some msg in a loop. 