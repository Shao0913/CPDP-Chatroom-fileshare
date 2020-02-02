//#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netdb.h>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
//#include <erron.h>
#include <pwd.h>
#include <sys/wait.h>
#include "config.h"
//using namespace std;

using std::string;
using std::vector;
using std::map;
using std::ifstream;
using std::ios;
using std::pair;
using std::cin;
using std::cout;
using std::endl;

class messenger_client;

pid_t Table[MAXPID];

void sig_int_handler(int sig_sum);

void *pthread_fun(void *arg);
void *ptherad_chat_fun(void *arg);

vector<string>* split_string(string s, const char *dev);

pthread_t pthread_id, pthread_chat_id;

class messenger_client
{
public:
	messenger_client(){}; 	//constructor
	messenger_client(const char *c); 	//parameterized constructor
	~messenger_client(){};		//deconstructor
	
	friend void *pthread_fun(void *arg);
	friend void *pthread_chat_fun(void *arg);
	friend void sig_int_handler(int sig_num);

	void print_start(int init_status);
	int init_socket();
	void loop();
	void menu_1();
	void menu_2();
	void menu_3();
	void login_loop();

private:
	string cli_fname;	//config file name
	int login_flag;		//reset after client logout
	int inchat_flag;	//reset after client quit chat room
	string inchat_room;
	map<string, string> config_file;	//config file content pair
	
	struct sockaddr_in server_address;
	int c2s_socket;		//socket
	string server_host;	//server host, from config file
	string local_ip;
	int server_port;	//server port, from config file
	
	int listen_port;
	int listen_socket;	//reset
	struct sockaddr_in listen_server_address;
	int c2c_con_num;	//reset
	int thread_finish_flag;	//reset

	int thread_chat_finish_flag;	//reset
	int download_flag;
	int upload_flag;
	int gobal_flag;

	//here is client information, need to clear after logout
	string client_name;
	vector<messenger_user> friend_list;
	vector<messenger_user> online_list;

	//here is chat room information, need to clear after exit room
	vector<string> room_list;
	vector<messenger_user> chat_member;
	vector<string> file_list;

	//client friend list info
	int c2c_online_num;	//reset
	vector<int> c2c_socket_vc;	//reset

	void read_client_filename(const char *fname);
	char *get_local_ip();
	string get_input();
	cmd_type parse_cmd(string buf);
	cmd_type parse_msg(const char *buf);

	void user_register();
	void user_login();
	void user_exit();

	//function used in login_loop()
	int init_friend_list();
	int init_listen_socket();
	void logout_clear();
	//void s2c_msg_handler();
	void s2c_msg_handler(const char *msg);
	void del_online_list(int socket);
	void del_online_list(string f_name);
	void c2c_msg_handler(const char *buf);

	//function chat_loop()
	void chat_loop(const char *chatroom);
	
	//function chat_loop() helper function
	int init_room_info(const char *chatroom);	//get how many chat room in server
	int init_room_member(string room_name);	//get how many people in chat room
	int init_room_listen_socket();
	void room_clear();
	void s2c_room_msg_handler(const char *msg);
	void add_room_list(string f_name);		//add member to chat_member
	void add_room_list(string f_name, string f_addr);
	void add_roomlist(string r_name);		//loading chatroom to roomlist 
	void del_room_list(int socket);
	void del_room_list(string user_name);
	void disp_room_list();
	void disp_member_list();
	void disp_file_list();

	//helper function of init_friend_list()
	void add2_friend_list(string f_name, string f_addr);
	void add2_friend_list(string f_name);
	void add2_online_list(string f_name, string f_addr);
	void add2_online_list(int socket, string f_name);
	void disp_friend_list();
	void disp_online_fd_list();

	//cmd function of thread handler funciton
	void user_enter_room(string cmd);
	void user_sendmsg(string cmd);
	void user_invite(string cmd);
	void user_accept(string cmd);
	void user_logout();

	//cmd function of chat thread handler function
	void sendmsg2_room(string cmd);
	void upload2_room(string cmd);
	void download4_room(string cmd);
	void edit(string cmd);
	void quit_room();

	//helper function of cmd function serious
	int con_friend(messenger_user *online_list, string user_target);

	//server to client msg, thread helper function
	void s2c_invite(const char *msg);
	void s2c_accept(const char *msg);
	void s2c_online(const char *msg);
	void s2c_offline(const char *msg);
	void s2c_msg(const char *msg);

	//server to client msg, thread helper function (chat room)
	void s2c_enter_room(const char *msg);
	void s2c_user_enter(const char *msg);
	void s2c_m_out(const char *msg);
	void s2c_msg_room(const char *msg);

	//get friend list and online friend list helper function
	messenger_user *get_friend_info(string f_name);
	messenger_user *get_online_info(string f_name);
	messenger_user *get_room_info(string r_name);
};

messenger_client::messenger_client(const char * c)
{
	this->cli_fname = string(c);
	this->read_client_filename(c);
	this->login_flag = 0;
	this->inchat_flag = 0;
	this->listen_port = 2129;
	this->c2c_con_num = 0;
	this->local_ip =string(get_local_ip());
	this->thread_finish_flag = 0;
	this->thread_chat_finish_flag = 0;
	this->download_flag = 0;
	this->upload_flag = 0;
	this->gobal_flag = 0;
}

void messenger_client::read_client_filename(const char * fname)
{
	#ifdef DEBUG
		printf("DEBUG: this is read_client_filename.\n");
	#endif
	ifstream fin(fname, ios::in);
	if(!fin)
	{
		printf("Open %s failed!\n", fname);
		exit(1);
	}
	string line;
	while(getline(fin,line))
	{
		vector<string> *token1 = split_string(line,":");
		if(token1->size() != 2)
		{
			fprintf(stderr, "Line: %d. Line: %d split error!\n", __LINE__, line.c_str());
			continue;
		}
		this->config_file.insert(pair<string, string>((*token1)[0], (*token1)[1]));
		delete token1;
	}
	fin.close();
	this->server_host = string((this->config_file)["servhost"]);
	this->server_port = atoi(string((this->config_file)["servport"]).c_str());
	#ifdef DEBUG
		printf("DEBUG: read_client_filename successful.\n");
	#endif
}

char *messenger_client::get_local_ip()
{
	#ifdef DEBUG
		printf("DEBUG: get_local_ip function.\n");
	#endif
	char host_name[128];
	struct hostent *host_entry;
	int hostname;
	gethostname(host_name, sizeof(host_name));
	host_entry = gethostbyname(host_name);
	if((sizeof(host_entry->h_addr_list)/sizeof(host_entry->h_addr_list[0])) < 1)
	{
		fprintf(stderr, "Line: %d. Get local ip failed!\n", __LINE__);
		exit(1);
	}
	//converts the internet host address in, char *inet_ntoa(struct in_addr in);
	this->local_ip = string(inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0])));
	#ifdef DEBUG
		printf("DEBUG: get_local_ip successful.\n");
	#endif
	return inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
}

string messenger_client::get_input()
{
	char buf[MAX_CMD_LEN];
	memset(buf, 0, MAX_CMD_LEN);
	cin.getline(buf, MAX_CMD_LEN);
	int i = 0;
	for( ; i < strlen(buf); ++i)
	{
		if(buf[i] != ' ')
			break;
	}
	return string(buf+i);
}

void messenger_client::print_start(int init_status)
{
	if(init_status)
	{
		printf("Connect to server failed! \nPlease make sure the IP and Port is right and the server is running.\n");
		printf("Server IP: %s\n", inet_ntoa(this->server_address.sin_addr));
		printf("Server Port: %d\n", ntohs(this->server_address.sin_port));
		return;
	}

	printf("\n**********************This is mychat **********************\n");
	printf("Connect successed! \nServer IP: %s\n", inet_ntoa(this->server_address.sin_addr));
	printf("Server Port: %d\n", ntohs(this->server_address.sin_port));
	printf("Local IP: %s\n", local_ip.c_str());
}

int messenger_client::init_socket()
{
	//create socket
	if((this->c2s_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		fprintf(stderr, "Line:%5d. Create socket failed.\n", __LINE__);
		return -1;
	}
	else
	{
		#ifdef DEBUG
			printf("DEBUG: create socket successful.\n");
		#endif
	}
	
	//bind socket and address
	//bzero(&(this->server_address),sizeof(this->server_address));
	memset(&(this->server_address), 0, sizeof(this->server_address));
	this->server_address.sin_family = AF_INET;
	this->server_address.sin_addr.s_addr = inet_addr(this->server_host.c_str());
	this->server_address.sin_port = htons(this->server_port);
	#ifdef DEBUG
		printf("DEBUG: bind socket and address successful.\n");
	#endif
	//connect server
	if(connect(this->c2s_socket, (struct sockaddr*)&this->server_address, sizeof(this->server_address)))
	{
		fprintf(stderr, "Line: %d. Connect server failed!\n", __LINE__);
		return -1;
	}
	else
	{
		#ifdef DEBUG
			printf("DEBUG: connect to server successful.\n");
		#endif
	}
	//success connect to server
	fprintf(stderr, "Connect with messenger server successed!\n");
	return 0;
}

void messenger_client::loop()
{
	while(1)
	{
		#ifdef DEBUG
			printf("DEBUG: login_flag is %d", this->login_flag);
		#endif
		if(this->login_flag)
		{
			this->menu_2();
		}
		else
		{
			this->menu_1();
		}
		string cmd_buf = get_input();
		if(cmd_buf.size() > 0)
		{
			#ifdef DEBUG
				printf("Line: %4d. loop, cmd_buf: [%s].\n", __LINE__, cmd_buf.c_str());
			#endif
			switch(parse_cmd(cmd_buf))
			{
				case CMD_REGISTER:
					user_register();
					break;
				case CMD_LOGIN:
					user_login();
					break;
				case CMD_EXIT:
					user_exit();
					return;
				case CMD_SENDMSG:
					;
				case CMD_INVITE:
					;
				case CMD_ACCEPT:
					;
				case CMD_LOGOUT:
					printf("Please login first.\n");
					break;
				default:
					printf("Unknown cmd. [%s]\n", cmd_buf.c_str());
					break;
			}
		}
	}
}

void messenger_client::menu_1()
{
	//the menu show before connect with server
	printf("\n---------------- MENU ----------------\n");
	printf("r       	register a new account\n");
	printf("l       	login to the server\n");
	printf("exit    	exit\n");
	printf("------------------------------------------------\n\n");
}

void messenger_client::menu_2()
{
	//the menu show after connect with server
	printf("\n---------------- MENU ----------------\n");
        printf("  m friend_name msg                     send msg to a friend\n");
        printf(" e chatroom_name pwd                  	enter chatroom and pwd\n");
        //printf(" up chatroom_name filename            up load a file to a chatroom\n");
        printf("  i new_friend_name msg                 inviate a new friend\n");
        printf("  a inviter_name msg                    accept a new friend request\n");
        printf("  logout                                logout from the server\n");
        printf("-----------------------------------------------\n\n"); 
}

void messenger_client::menu_3()
{
	//the menu show after enter chat room
	printf("\n---------------- MENU ----------------\n");
        printf("  mr chatroom_name msg                     send msg to chatroom\n");
        printf(" u chatroom_name file                    upload a file to chatroom\n");
	printf("  d chatroom_name file                    download a file from chatroom\n");
        printf("edit chatroom_name file			edit file from server\n");
	printf("  quit                                exit from chatroom\n");
        printf("-----------------------------------------------\n\n");
}

cmd_type messenger_client::parse_cmd(string buf)
{
	int div = buf.find(' ');
	string cmd = buf.substr(0, div);
	if(cmd == "r")
	{
		return CMD_REGISTER;
	}
	if(cmd == "l")
	{
		return CMD_LOGIN;
	}
	if(cmd == "exit")
	{
		return CMD_EXIT;
	}
	if(cmd == "m")
	{
		return CMD_SENDMSG;
	}
	if(cmd == "i")
	{
		return CMD_INVITE;
	}
	if(cmd == "a")
	{
		return CMD_ACCEPT;
	}
	if(cmd == "logout")
	{
		return CMD_LOGOUT;
	}
	if(cmd == "e")
	{
		return CMD_ENTER_ROOM;
	}
	if(cmd == "mr")
	{
		return CMD_MSG_ROOM;
	}
	if(cmd == "u")
	{
		return CMD_UPLOAD;
	}
	if(cmd == "d")
	{
		return CMD_DOWNLOAD;
	}
	if(cmd == "edit")
	{
		return CMD_EDIT;
	}
	if(cmd == "quit")
	{
		return QUIT_ROOM;
	}
	return CMD_UNKNOWN;
}

cmd_type messenger_client::parse_msg(const char *buf) {
	if (strncmp(buf, "CMD_INVITE", strlen("CMD_INVITE")) == 0) 
	{
		return CMD_INVITE;
	}
	if (strncmp(buf, "CMD_ACCEPT", strlen("CMD_ACCEPT")) == 0) 
	{
		return CMD_ACCEPT;
	}
	if (strncmp(buf, "CMD_FD_ONLINE", strlen("CMD_FD_ONLINE")) == 0) 
	{
		return CMD_FD_ONLINE;
	}
	if (strncmp(buf, "CMD_FD_OFFLINE", strlen("CMD_FD_OFFLINE")) == 0) 
	{
		return CMD_FD_OFFLINE;
	}
	if (strncmp(buf, "CMD_C2CMSG", strlen("CMD_C2CMSG")) == 0) 
	{
		return CMD_C2CMSG;
	}
	if (strncmp(buf, "CMD_S2CMSG", strlen("CMD_S2CMSG")) == 0) 
	{
		return CMD_S2CMSG;
	}
	if (strncmp(buf, "CMD_ENTER_SUCCESSED", strlen("CMD_ENTER_SUCCESSED")) == 0)
        {
                return CMD_ENTER_SUCCESSED;
        }
	if (strncmp(buf, "CMD_USER_ENTER", strlen("CMD_USER_ENTER")) == 0)
        {
                return CMD_USER_ENTER;
        }
	if (strncmp(buf, "CMD_M_OUT", strlen("CMD_M_OUT")) == 0)
        {
                return CMD_M_OUT;
        }
	if (strncmp(buf, "CMD_MSG_ROOM", strlen("CMD_MSG_ROOM")) == 0)
        {
                return CMD_MSG_ROOM;
        }
	return CMD_UNKNOWN;
}

void messenger_client::user_register()
{
	printf("Please input your accout name and password for register.\n");
	printf("eg: user_name my_password\n");
	string cmd_buf = get_input();
	string user_name = cmd_buf.substr(0, cmd_buf.find(' '));
	string user_pwd = cmd_buf.substr(cmd_buf.find(' ')+1);
	int socket_len = 0;
	char send_buf[MSG_LEN], recv_buf[MSG_LEN];
	sprintf(send_buf, "CMD_REGISTER|%s|%s|", user_name.c_str(), user_pwd.c_str());
	socket_len=send(this->c2s_socket, send_buf, strlen(send_buf), 0);
	if(socket_len <= 0)
	{
		fprintf(stderr, "Line: %d. Send msg failed!\n", __LINE__);
		return;
	}
	printf("Waiting for the server response...\n");
	socket_len = 0;
	socket_len = recv(this->c2s_socket, recv_buf, MSG_LEN, 0);
	if (socket_len >0)
	{
		if(strncmp(recv_buf, "CMD_REGISTER_SUCCESSED", strlen("CMD_REGISTER_SUCCESSED"))==0)
		{
			printf("Register successed!\n");
			printf("Here is your username and password:\nusername:%s\npassword:%s", user_name.c_str(), user_pwd.c_str());
			return;
		}
		else
		{
			printf("The username [%s] is already exist, re-enter.\n", user_name.c_str());
			return;
		}
	}
	else
	{
		fprintf(stderr, "Line: %d. Recv msg failed!\n", __LINE__);
		return;
	}
}

void messenger_client::user_login()
{
	if(login_flag)
	{
		printf("Your already login at [%s]\n", (this->client_name).c_str());
		return;
	}
	printf("Use following format type in your accout name and password to login.\n");
	printf("eg: user_name password\n");
	string cmd_buf = get_input();
	string user_name = cmd_buf.substr(0, cmd_buf.find(' '));
	string user_pwd = cmd_buf.substr(cmd_buf.find(' ')+1);
	int socket_len = 0;
	char send_buf[MSG_LEN], recv_buf[MSG_LEN];
	sprintf(send_buf, "CMD_LOGIN|%s|%s|%s|", user_name.c_str(), user_pwd.c_str(), (this->local_ip).c_str());
	socket_len = send(this->c2s_socket, send_buf, strlen(send_buf), 0);
	if(socket_len <= 0)
	{
		fprintf(stderr, "Line: %d. Send msg failed!\n", __LINE__);
		return;
	}
	printf("Waiting for the server response...\n");
	socket_len = 0;
	socket_len = recv(this->c2s_socket, recv_buf, MSG_LEN, 0);
	if(socket_len > 0)
	{
		if(strncmp(recv_buf, "CMD_LOGIN_SUCCESSED", strlen("CMD_LOGIN_SUCCESSED"))==0)
		{
			printf("Login successed!\n");
			this->login_flag = 1;
			this->client_name = user_name;
			login_loop();
			return;
		}
		else if (strncmp(recv_buf, "CMD_NOWONLINE", strlen("CMD_NOWONLIN"))==0)
		{
			printf("The user is online now, maybe you lost your password?\n");
			return;
		}
		else
		{
			printf("the password is wrong, please re-enter.\n");
			return;
		}
	}
	else
	{
		fprintf(stderr, "Line: %d. recv msg failed!\n", __LINE__);
		return;
	}
}

void messenger_client::user_exit()
{
	int socket_len = 0;
	char send_buf[MSG_LEN], recv_buf[MSG_LEN];
	sprintf(send_buf, "CMD_EXIT|");
	if(this->c2s_socket > 0)
	{
		socket_len = send(this->c2s_socket, send_buf, strlen(send_buf),0);
	}
	
	if(this->c2s_socket > 0)
	{
		shutdown(this->c2s_socket, SHUT_RDWR);
		close(this->c2s_socket);
		c2s_socket = -1;
	}
	if(this->listen_socket > 0)
	{
		shutdown(this->listen_socket, SHUT_RDWR);
		close(this->listen_socket);
		listen_socket = -1;
	}

	printf("\nmessenger_client shutdown!\n");
	exit(0);
}

void messenger_client::login_loop()
{
	#ifdef DEBUG
		printf("DEBUG: login_loop.\n");
	#endif
	int flag = 0;
	//initial friends list from server
	flag = init_friend_list();
	if(flag)
	{
		fprintf(stderr, "Line: %d. Request friend list failed!\n", __LINE__);
		return;
	}
	else
	{
		#ifdef DEBUG
			printf("DEBUG: Request friend list successed!\n");
		#endif
		//return;
	}
	
	//listen to other clients
	if(init_listen_socket())
	{
		fprintf(stderr, "Line: %d. Init listen socket failed!\n", __LINE__);
		return;
	}
	else
	{
		#ifdef DEBUG
			printf("DEBUG: init_listen_socket success.\n");
		#endif
	}

	//create a thread to handle the input cmd from user
	this->thread_finish_flag =0;
	flag = pthread_create(&pthread_id, NULL, pthread_fun, NULL);
	if(flag !=0)
	{
		fprintf(stderr, "Line: %d. Pthread_create failed!\n", __LINE__);
		return;
	}
	
	//select socket from the server, new connection, and other clients
	int i = 0, new_client_fd;
	socklen_t new_client_size;
	char recv_buf[MSG_LEN];
	vector<int>::iterator vc_it;
	fd_set fdss,allset,origset;
	struct timeval timeout_val;
	struct sockaddr_in new_cli_addr;
	this->c2c_online_num = 0;
	this->c2c_socket_vc.clear();

	while(1)
	{

		while(gobal_flag)
		{
			;
		}
		if(this->thread_finish_flag || listen_socket < 0 || c2s_socket < 0)
		{
			logout_clear();
			return;
		}
		#ifdef DEBUG
			//printf("DEBUG: after if login flag = %d.\n", this->login_flag);
		#endif
		//clear the socket set
		FD_ZERO(&allset);
		//add client to server and listen socket to set
		FD_SET(this->c2s_socket, &allset);
		FD_SET(this->listen_socket, &allset);
		//setup timeout value
		timeout_val.tv_sec = 0;
		timeout_val.tv_usec = 100000;
		//add all online client to set
		for(i = 0; i < this->c2c_online_num; ++i)
		{
			FD_SET(this->c2c_socket_vc[i], &allset);
		}

		flag = select(FD_SETSIZE, &allset, NULL, NULL, &timeout_val);
		if(flag < 0)
		{
			fprintf(stderr, "Line: %d. Select failed!\n", __LINE__);
			break;
		}
		else if (flag== 0)
		{
			//fprintf(stderr, "Line: %d. Select timeout.\n", __LINE__);
			continue;
		}	
		
		//check if server send something
		if(FD_ISSET(this->c2s_socket, &allset))
		{
			memset(recv_buf, 0, sizeof(recv_buf));
			flag = recv(this->c2s_socket, recv_buf, sizeof(recv_buf), 0);
			//server is close
			if(flag <= 0)
			{
				return;
			}
			//receive data
			recv_buf[flag] = '\0';
			//server send to client messeng handler
			#ifdef DEBUG
				printf("DEBUG: receive msg from server: %s\n",recv_buf);
			#endif
			s2c_msg_handler(recv_buf);
			continue;
		}

		//check a new connection from a client
		if(FD_ISSET(this->listen_socket, &allset))
		{
			new_client_size = sizeof(new_cli_addr);
			new_client_fd = accept(this->listen_socket, (struct sockaddr*)&new_cli_addr, &new_client_size);
			if(new_client_fd == -1)
			{
				fprintf(stderr, "Line: %d. Accept socket failed!\n", __LINE__);
				continue;
			}
			//add to c2c_socket_vc
			if(this->c2c_online_num >= MAX_C2C)
			{
				fprintf(stderr, "Line: %d. Max connection between clients. Reject this connection request.\n", __LINE__);
				continue;
			}
			this->c2c_socket_vc.push_back(new_client_fd);
			++this->c2c_online_num;
			continue;
		}
		
		//check for a established client connection, if any client is disconnected
		vc_it = this->c2c_socket_vc.begin();
		for( ; vc_it != this->c2c_socket_vc.end(); ++vc_it)
		{
			if(FD_ISSET(*vc_it, &allset))
			{
				memset(recv_buf, 0, sizeof(recv_buf));
				flag = recv(*vc_it, recv_buf, sizeof(recv_buf), 0);
				//client is close
				if(flag <=0)
				{
					shutdown(*vc_it, SHUT_RDWR);
					close(*vc_it);
					FD_CLR(*vc_it, &allset);
					this->c2c_socket_vc.erase(vc_it);
					--this->c2c_con_num;
					del_online_list(*vc_it);
					break;
				}
				else
				{
					//receive data
					recv_buf[flag] = '\0';
					c2c_msg_handler(recv_buf);
				}
			}
		}
	}
}	

int messenger_client::init_friend_list()
{
	#ifdef DEBUG
		printf("DEBUG: friend list init function.\n");
	#endif
	
	int socket_len =0, flag =0;
	char send_buf[MAX_LEN], recv_buf[MAX_LEN];
	sprintf(send_buf, "CMD_REQUEST_FRIEND_ID|%s|", (this->client_name).c_str());
	socket_len = send(this->c2s_socket, send_buf, strlen(send_buf), 0);
	if(socket_len <=0)
	{
		fprintf(stderr, "Line: %d. Send msg failed!\n", __LINE__);
		return -1;
	}
	else
	{
		#ifdef DEBUG
			printf("socket send successful.\n");
		#endif
	}
	
	printf("Waiting for loading the friend list...\n");
	memset(recv_buf, 0, sizeof(recv_buf));
	socket_len = 0;
	socket_len= recv(this->c2s_socket, recv_buf, MSG_LEN, 0);
	while(socket_len>0)
	{
		#ifdef DEBUG
			printf("DEBUG: Receive buf is [%s]\n", recv_buf);
		#endif
		if(strncmp(recv_buf, "CMD_END", strlen("CMD_END")) == 0)
		{
			#ifdef DEBUG
				printf("DEBUG: receive CMD_END.\n");
			#endif
			break;
		}

		if(strncmp(recv_buf, "CMD_FD_ONLINE", strlen("CMD_FD_ONLINE")) == 0)
		{
			#ifdef DEBUG
				printf("DEBUG: receive CMD_FD_ONLINE.\n");
            		#endif
			char buf_tmp[MSG_LEN], *f_cmd, *f_name, *f_addr;
			memset(buf_tmp, 0, MSG_LEN);
			memcpy(buf_tmp, recv_buf, strlen(recv_buf));
			f_cmd = strtok(buf_tmp, "|");
			f_name = strtok(NULL, "|");
			f_addr = strtok(NULL, "|");
			#ifdef DEBUG
				printf("DEBUG: request friend information: f_cmd=%s, f_name=%s, f_addr= %s.\n", f_cmd, f_name, f_addr);
			#endif
			add2_friend_list(string(f_name), string(f_addr));
			add2_online_list(string(f_name), string(f_addr));
		}
		
		if(strncmp(recv_buf, "CMD_FD_OFFLINE", strlen("CMD_FD_OFFLINE")) == 0)
		{
                        #ifdef DEBUG
                                printf("DEBUG: receive CMD_FD_OFFLINE.\n");
                        #endif
			char buf_tmp[MSG_LEN], *f_cmd, *f_name;
			memset(buf_tmp, 0, MSG_LEN);
			memcpy(buf_tmp, recv_buf, strlen(recv_buf));
			f_cmd = strtok(buf_tmp, "|");
			f_name = strtok(NULL, "|");
			#ifdef DEBUG
				printf("DEBUG: request friend information: f_cmd=%s, f_name=%s.\n", f_cmd, f_name);
			#endif

			while(f_name != NULL)
			{
				add2_friend_list(string(f_name));
				f_name = strtok(NULL, "|");
			}
			break;
		}
		memset(recv_buf, 0, sizeof(recv_buf));
		socket_len = 0;
		socket_len = recv(this->c2s_socket, recv_buf, MSG_LEN, 0);
	}
	
	if(socket_len <= 0)
	{
		fprintf(stderr, "Line: %d. recv msg failed!\n", __LINE__);
		return -1;
	}
	disp_friend_list();
	disp_online_fd_list();
	return 0;
}

void messenger_client::add2_friend_list(string f_name, string f_addr)
{
	//clean up address make sure the format is right
	int i = 0;
	char buf[64], *ptr;
	const char *c_ptr;
	ptr = buf;
	c_ptr = f_addr.c_str();
	for(i=0; i< strlen(c_ptr); ++i)
	{
		if(isdigit(*(c_ptr+i)) || *(c_ptr+i) == '.')
		{
			(*ptr) = *(c_ptr+i);
			++ptr;
		}
	}
	(*ptr) = '\0';
	#ifdef DEBUG
		printf("clean ip address: %s\n", ptr);
	#endif

	messenger_user client;
	client.name = f_name;
	client.addr = string(buf);
	client.socket = -1;
	for( i = 0; i<this->friend_list.size();++i)
	{
		if((this->friend_list)[i].name == f_name)
			return;
	}
	this->friend_list.push_back(client);
}

void messenger_client::add2_friend_list(string f_name)
{
	int i = 0;
	messenger_user client;
	client.name = f_name;
	client.socket = -1;
	for(i = 0; i < this->friend_list.size(); ++i)
	{
		if((this->friend_list)[i].name == f_name)
			return;
	}
	this->friend_list.push_back(client);
}

void messenger_client::add2_online_list(string f_name, string f_addr)
{
	//clean up address make sure the format is right
	int i = 0;
        char buf[64], *ptr;
        const char *c_ptr;
        ptr = buf;
        c_ptr = f_addr.c_str();
        for(i=0; i< strlen(c_ptr); ++i)
        {
                if(isdigit(*(c_ptr+i)) || *(c_ptr+i) == '.')
                {
                        (*ptr) = *(c_ptr+i);
                        ++ptr;
                }
        }
        (*ptr) = '\0';
        #ifdef DEBUG
                printf("DEBUG:clean ip address: %s, and f_name is %s\n", ptr, f_name.c_str());
        #endif

        messenger_user client;
        client.name = f_name;
        client.addr = string(buf);
        client.socket = -1;
        for( i = 0; i<this->online_list.size();++i)
        {
		#ifdef DEBUG
			//printf("DEBUG: ");
		#endif
                if((this->online_list)[i].name == f_name)
                {
			#ifdef DEBUG
				printf("DEBUG: check loop [%d] is true and return.\n", i);
			#endif
		        return;
		}
        }
        this->online_list.push_back(client);
	#ifdef DEBUG
		printf("DEBUG: online_list push back success.size of list is %d\n", online_list.size());
	#endif

}

void messenger_client::add2_online_list(int socket, string f_name)
{
	vector<messenger_user>::iterator it = this->online_list.begin();
	for (; it != this->online_list.end(); ++it) 
	{
		if (it->name == f_name) 
		{
			if (it->socket < 0) 
			{
				it->socket = socket;
			}
			return;
		}
	}

}

void messenger_client::disp_friend_list()
{
	printf("\n **************Friends***************:\n ");
	if(this->friend_list.size() == 0)
	{
		printf("You don't have friends.\n");
		return;
	}

	int i = 0;
	for(i = 0; i < this->friend_list.size(); ++i)
	{
		printf("[%s], ", (this->friend_list)[i].name.c_str());
	}
	printf("\n");
}

void messenger_client::disp_online_fd_list()
{
	printf("\n ******************Online*****************:\n ");
	if(this->online_list.size() == 0)
	{
		printf("You don't have friends online.\n");
		return;
	}

	int i = 0;
	for(i = 0; i < this->online_list.size(); ++i)
	{
		printf("[%s], ", (this->online_list)[i].name.c_str());
	}
	printf("\n");
}

int messenger_client::init_listen_socket()
{
	//create socket
	this->listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(this->listen_socket == -1)
	{
		fprintf(stderr, "Line:%5d. Create socket failed.\n", __LINE__);
		user_exit();
		printf("user exiting!\n");
		exit(0);
	}	

	//bind socket and address
	memset(&(this->listen_server_address), 0, sizeof(this->listen_server_address));
	this->listen_server_address.sin_family = AF_INET;
	this->listen_server_address.sin_addr.s_addr = inet_addr(local_ip.c_str());
	this->listen_server_address.sin_port = htons(this->listen_port);

	//check bind
	int i = 1;
	setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int));
	if(bind(this->listen_socket, (struct sockaddr*)&(this->listen_server_address), sizeof(this->listen_server_address)) == -1)
	{
		fprintf(stderr, "Line:%5d. Bind socket failed.\n", __LINE__);
		user_exit();
		exit(0);
	}
	
	//listen to clients
	if(listen(this->listen_socket, MAX_C2C) == -1)
	{
		fprintf(stderr, "Line:%5d. Listen socket failed\n", __LINE__);
		user_exit();
		exit(0);
		return -1;
	}
	return 0;
}

void messenger_client::logout_clear()
{
	fprintf(stderr, "Line: %d. logout memery reset.\n", __LINE__);
	int i = 0;
	for(i = 0; i<this->c2c_con_num; ++i)
		close(this->c2c_socket_vc[i]);
	this->c2c_con_num = 0;
	vector<int>().swap(this->c2c_socket_vc);
	if(listen_socket >= 0)
	{
		shutdown(listen_socket, SHUT_RDWR);
		close(this->listen_socket);
	}
	this->listen_socket = -1;
	this->login_flag = 0;
	vector<messenger_user>().swap(this->friend_list);
	vector<messenger_user>().swap(this->online_list);
	this->thread_finish_flag = 0;
	fprintf(stderr, "Line: %d. logout memreset finished.\n", __LINE__);
}

void messenger_client::s2c_msg_handler(const char *msg)
{
	#ifdef DEBUG
		printf("DEBUG: this is s2c_msg_handler getting msg: [%s].\n", msg);
	#endif
	switch(parse_msg(msg))
	{
		case CMD_INVITE:
			s2c_invite(msg);
			break;
		case CMD_ACCEPT:
			s2c_accept(msg);
			break;
		case CMD_FD_ONLINE:
			s2c_online(msg);
			break;
		case CMD_FD_OFFLINE:
			s2c_offline(msg);
			break;
		case CMD_S2CMSG:
			s2c_msg(msg);
			break;
		default:
			printf("Unknown server to client cmd [%s], try s2c_chat_msg_handler.\n", msg);
			if(inchat_flag == 0)
			{
				fprintf(stderr, "Line: %d. Not yet enter a chatroom, if it's chatroom cmd, then chatroom cmd is not avaible, please enter one!\n", __LINE__);
				//s2c_room_msg_handler(msg);
			}
			else
			{
				s2c_room_msg_handler(msg);
			}
			break;
	}
}

void messenger_client::del_online_list(int socket)
{
	vector<messenger_user>::iterator it;
	for(it = this->online_list.begin(); it != this->online_list.end(); ++it)
	{
		if(it->socket == socket)
		{
			this->online_list.erase(it);
			#ifdef DEBUG
				printf("Line: %4d. del_online_list, [%d] deleted.\n", __LINE__, socket);
			#endif
			break;
		}
	}
}

void messenger_client::del_online_list(string f_name)
{
	vector<messenger_user>::iterator it;
	for (it = this->online_list.begin(); it != this->online_list.end(); ++it)
	{
		if (it->name == f_name)
		{
			if (it->socket > 0) 
			{
				shutdown(it->socket, SHUT_RDWR);
				close(it->socket);
			}
			this->online_list.erase(it);
			break;
		}
	}
}

void messenger_client::c2c_msg_handler(const char *buf)
{
	char *user_cmd, *user_name, *user_msg, str[strlen(buf) + 1];
	memcpy(str, buf, strlen(buf));
	user_cmd = strtok(str, "|");
	user_name = strtok(NULL, "|");
	user_msg = strtok(NULL, "|");
	printf("\nclient_name[%s] >>\n%s\n\n", user_name, user_msg);
}


void messenger_client::user_sendmsg(string cmd)
{
	int pos1, pos2, flag, f_socket;
	string user_target, user_msg;
	pos1 = cmd.find(' ');
	pos2 = cmd.find(' ', pos1 + 1);
	user_target = cmd.substr(pos1 + 1, pos2 - pos1 - 1);
	user_msg    = cmd.substr(pos2 + 1);

	#ifdef DEBUG
		printf("DEBUG:user_sendmsg firendlist size: %d", friend_list.size());
	#endif

	messenger_user *f_list = get_friend_info(user_target);
	if(f_list == NULL)
	{
		printf("%s is not in your friend list.\n", user_target.c_str());
		return;
	}
	delete f_list;

	messenger_user *online_list = get_online_info(user_target);
	if(online_list == NULL)
	{
		printf("[%s] is not online right now.\n", user_target.c_str());
		return;
	}

	if(online_list->socket == -1)
	{
		f_socket = con_friend(online_list, user_target);
		if(f_socket == -1)
		{
			printf("Connection to friend [%s] failed!\n", user_target.c_str());
			return;
		}
	}
	else
	{
		f_socket = online_list->socket;
	}

	char send_buf[MSG_LEN];
	sprintf(send_buf, "CMD_C2CMSG|%s|%s|", (this->client_name).c_str(), user_msg.c_str());
	flag = send(f_socket, send_buf, strlen(send_buf), 0);
	if(flag <= 0)
	{
		fprintf(stderr, "Line: %d. Send msg to friend [%s] failed!\n", __LINE__, user_target.c_str());
	}
	delete online_list;
}

void messenger_client::user_invite(string cmd)
{
	#ifdef DEBUG
		printf("DEBUG: user_invite.\n");
	#endif
	int pos1, pos2, flag, f_socket;
	string user_target, user_msg;
	pos1 = cmd.find(' ');
	pos2 = cmd.find(' ', pos1 + 1);
	user_target = cmd.substr(pos1 + 1, pos2 - pos1 - 1);
	user_msg    = cmd.substr(pos2 + 1);

	char send_buf[MSG_LEN];
	sprintf(send_buf, "CMD_INVITE|%s|%s|%s|%s|", this->client_name.c_str(), (this->local_ip).c_str(), user_target.c_str(), user_msg.c_str());
	flag = send(this->c2s_socket, send_buf, strlen(send_buf), 0);
	if (flag <= 0) 
	{
		fprintf(stderr, "Line: %d. Send msg failed!\n", __LINE__);
	}

	/*messenger_user *f_info = get_friend_info(user_target);
	if(f_info == NULL)
	{
		printf("%s is not in your friend list.\n", user_target.c_str());
		return;
	}
	delete f_info;

	messenger_user *online_info = get_online_info(user_target);
	if(online_info == NULL)
	{
		printf("[%s] is not online.\n", user_target.c_str());
		return;
	}

	if(online_info->socket == -1)
	{
		f_socket = con_friend(online_info, user_target);
		if(f_socket == -1)
		{
			printf("Connection with friend [%s] fail!\n", user_target.c_str());
			return;
		}
	}
	else
	{
		f_socket = online_info->socket;
	}

	char send_buf[MSG_LEN];
	sprintf(send_buf, "CMD_C2CMSG|%s|%s|", (this->client_name).c_str(), user_msg.c_str());
	flag = send(f_socket, send_buf, strlen(send_buf), 0);
	if(flag <= 0)
	{
		fprintf(stderr, "Line: %d. Send msg to [%s] failed!\n", __LINE__, user_target.c_str());
	}
	delete online_info;*/
}

void messenger_client::user_accept(string cmd)
{
	int pos1, pos2, flag;
	string user_target, user_msg;
	pos1 = cmd.find(' ');
	pos2 = cmd.find(' ', pos1 + 1);
	user_target = cmd.substr(pos1 + 1, pos2 - pos1 - 1);
	user_msg    = cmd.substr(pos2 + 1);

	char send_buf[MSG_LEN];
	sprintf(send_buf, "CMD_ACCEPT|%s|%s|%s|%s|", this->client_name.c_str(), (this->local_ip).c_str(), user_target.c_str(), user_msg.c_str());
	flag = send(this->c2s_socket, send_buf, strlen(send_buf), 0);
	if(flag <= 0)
	{
		fprintf(stderr, "Line: %d. Send msg to [%s] failed!\n", __LINE__, user_target.c_str());
	}
}

void messenger_client::user_logout()
{
	int flag = 0;
	char send_buf[MSG_LEN];
	sprintf(send_buf, "CMD_LOGOUT|%s|", this->client_name.c_str());
	flag = send(this->c2s_socket, send_buf, strlen(send_buf), 0);
	if(flag <= 0)
	{
		fprintf(stderr, "Line: %d. Send msg to server failed!\n", __LINE__);
	}
	printf("Already log out from the server.\n");
	this->login_flag = 0;
	logout_clear();
}

int messenger_client::con_friend(messenger_user *online_list, string f_name)
{
	int flag = 0, new_client_fd, new_client_size;
	struct sockaddr_in new_client_address;
	// create socket;
	new_client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (new_client_fd == -1) 
	{
		fprintf(stderr, "Line:%5d. Create socket failed.\n", __LINE__);
		return -1;
	}
	// bind socket and address;
	int reuse = 1;
	setsockopt(new_client_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
	int cur_port = listen_port;
	memset(&(new_client_address), 0, sizeof(new_client_address));
	new_client_address.sin_family = AF_INET;
	new_client_address.sin_addr.s_addr = inet_addr(online_list->addr.c_str());
	new_client_address.sin_port = htons(cur_port);
	// connect remote client;
	if (connect(new_client_fd, (struct sockaddr*)&new_client_address, sizeof(new_client_address)) < 0) 
	{
		fprintf(stderr, "Line: %d. Connect remote client [%s] [%s] failed!\n", __LINE__, online_list->name.c_str(), online_list->addr.c_str());
		return -1;
	}
	// add to online list;
	add2_online_list(new_client_fd, online_list->name);
	return new_client_fd;
}


void messenger_client::s2c_invite(const char *msg)
{
	char *user_cmd, *user_name, *user_addr, *user_msg, str[strlen(msg) + 1];
	memcpy(str, msg, strlen(msg));
	user_cmd = strtok(str, "|");
	user_name = strtok(NULL, "|");
	user_addr = strtok(NULL, "|");
	user_msg = strtok(NULL, "|");
	printf("[%s] invite you as friend. >>\n%s\n", user_name, user_msg);
	printf("You can use command \"a inviter_name some_msg\" to accecpt this invitation.\n");

}

void messenger_client::s2c_accept(const char *msg)
{
	char *user_cmd, *user_name, *user_addr, *user_msg, str[strlen(msg) + 1];
	memcpy(str, msg, strlen(msg));
	user_cmd = strtok(str, "|");
	user_name = strtok(NULL, "|");
	user_addr = strtok(NULL, "|");
	user_msg = strtok(NULL, "|");
	printf("[%s] accepted your invitation, you and [%s] are friend now! >>\n%s\n", user_name, user_name, user_msg);
	//add2_friend_list
}

void messenger_client::s2c_online(const char *msg)
{
	#ifdef DEBUG
		printf("DEBUG: s2c_online.\n");
	#endif
	char *user_cmd, *user_name, *user_addr,str[strlen(msg) + 1];
	memcpy(str, msg, strlen(msg));
	user_cmd = strtok(str, "|");
	user_name = strtok(NULL, "|");
	user_addr = strtok(NULL, "|");
	printf("Your friend [%s] is online now.\n", user_name);
	add2_friend_list(user_name, user_addr);
	add2_online_list(user_name, user_addr);
	#ifdef  DEBUG
		printf("DEBUG: friend_list size %d, online_list size %d", friend_list.size(), online_list.size());
	#endif
	disp_friend_list();
	disp_online_fd_list();
}

void messenger_client::s2c_offline(const char *msg)
{
	char *user_cmd, *user_name, str[strlen(msg) + 1];
	memcpy(str, msg, strlen(msg));
	user_cmd = strtok(str, "|");
	user_name = strtok(NULL, "|");
	printf("Your friend [%s] is offline now.\n", user_name);
	del_online_list(string(user_name));
	disp_friend_list();
	disp_online_fd_list();
}

void messenger_client::s2c_msg(const char *msg)
{
	char *user_cmd, *user_msg, str[strlen(msg) + 1];
	memcpy(str, msg, strlen(msg));
	user_cmd = strtok(str, "|");
	user_msg = strtok(NULL, "|");
	printf("\n*****Server send a msg >>>>  %s\n\n", user_msg);
}

messenger_user *messenger_client::get_friend_info(string f_name)
{
	int i = 0;
	for(i = 0; i < this->friend_list.size(); ++i)
	{
		if((this->friend_list)[i].name == f_name)
		{
			messenger_user *tmp = new messenger_user((this->friend_list)[i]);
			#ifdef DEBUG
				printf("Get friend info [%s] [%s]\n", tmp->name.c_str(), tmp->addr.c_str());
			#endif
			return tmp;
		}
	}
	return NULL;
}

messenger_user *messenger_client::get_online_info(string f_name)
{
	int i = 0;
	for(i = 0; i < this->online_list.size(); ++i)
	{
		if((this->online_list)[i].name == f_name){
			messenger_user *tmp = new messenger_user((this->online_list)[i]);
			#ifdef DEBUG
				printf("Get friend info [%s] [%s]\n", tmp->name.c_str(), tmp->addr.c_str());
			#endif
			return tmp;
		}
	}
	return NULL;
}
