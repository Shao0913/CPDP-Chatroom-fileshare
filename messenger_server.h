#ifndef __MESSENGER_SERVER_H_
#define __MESSENGER_SERVER_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <sys/types.h>
#include <strings.h>
#include <pthread.h>
#include "config.h"

using namespace std;

//using std::string;
//using std::vector;
//using std::map;
//using std::ifstream;
//using std::pair;
//using std::ios;
//using std::cout;
//using std::endl;
//using std::getline;

class messenger_server;



struct pthread_arg
{
	messenger_server *server;
	int *socket;
};

void sig_int_handler(int sig_num);

void *pthread_fun(void *arg);

vector<string>* split_string(string s, const char *dev);

class messenger_server
{
public:
	messenger_server(){};	//default constructor
	messenger_server(const char *user_info, const char *config, const char *room_info, const char *filelist_info);	//parameter constructor
	~messenger_server();	//deconstructor
	void exit_server();

	int init_socket();	//initial socket
	void loop();		//loop waiting for receive

	//helper function(printf)
	void print_local_ip();

	friend void sig_int_handler(int sig_num);
	friend void *pthread_fun(void *arg);
	
	
private:
	string user_file_name;	//user_info_file name
	string config_file_name;	//configuration_file name
	string room_file_name;		//chat room configration file name
	string filelist_file_name;	//chat room file list configration file name
	int total_login_users;		//number of login users

	pthread_t pthread_id[MAX_THREAD_NUM];	//thread list declare
	int num_thread;			//i'th number of thread

	map<string, string> user_pwd;	//user and password pair
	map<string, vector<string> > user_contact;	//user concact list
	map<string, string> confile;

	map<string, string> room_pwd;			//chat room with password
	map<string, vector<string> > room_member;	//chat room with member info
	map<string, vector<string> > room_file;		//chat room with file list info
	
	vector<messenger_user> login_list;	//login client list

	int server_socket;
	int server_port;	//server port: read from config file
	string local_ip;	//server local ip
	struct sockaddr_in server_address;

	//helper function(printf)
	//void print_local_ip();

	char *get_local_ip();
	void read_user_filename(const char *fname);
	void read_config_filename(const char *fname);
	void read_user_chatroom(const char *fname);
	void read_room_filelist(const char *fname);
	cmd_type parse_cmd(const char *line);
	void user_register(const char *line, int socket);
	void user_login(const char *line, int socket);
	void user_logout(const char *line, int socket);
	void user_invite(const char *line, int socket);
	void user_accept(const char *line, int socket);
	void user_friend(const char *line, int socket);
	void user_exit(const char *line, int socket);

	//cmd or msg handler for chatroom
	void user_enter_room(const char *line, int socket);
	void user_msg_room(const char *line, int socket);
	void user_upload(const char *line, int socket);
	void user_download(const char *line, int socket);
	void user_quit_room(const char *line, int socket);
	void user_roominfo(const char *line, int socket);
	void user_edit_done(const char *line, int socket);
	
	//helper function
	void add2_login_list(const char *user_name, const char *user_pwd, const char *user_addr, int socket);
	messenger_user *get_user_info(int socket);
	messenger_user *get_user_info(const char *name);
	vector<string> get_friends_info(const char *name);
	void add_user_pwd(const char *user_name, const char *user_pwd);
	void add_user_contact(const char *user_name);
	void add_friend(const char *name, const char *fname);
	void remove_from_login_list(int socket);
	void remove_from_login_list(const char *user_name);

	//helper function for chat room
	int room_pwd_check(const char *chatroom, const char *room_msg);
	int room_member_check(const char *chatroom, const char *user_name);
	int check_file_status(const char *user_name, const char *file_name, const char *room_name);		//check if file in current room file list
	void add2_room_member(const char *room_name, const char *user_name);
	vector<string> get_member_info(const char *room_name);
	void add_room_file(const char *room_name, const char *fname);
	void remove_room_member(const char *user_name, const char *room_name);
	//void remove_file_list(const char *fname);
};
#endif

messenger_server::messenger_server(const char *user_info, const char *config, const char *room_info, const char *filelist_info)
{
	this -> user_file_name = string(user_info);
	this -> config_file_name = string(config);
	this -> room_file_name = string(room_info);
	this -> filelist_file_name = string(filelist_info);
	read_user_filename(user_info);
	read_config_filename(config);
	read_user_chatroom(room_info);
	read_room_filelist(filelist_info);
	this -> total_login_users = 0;
	this -> num_thread = 0;
	this -> local_ip = string(get_local_ip());
}

void messenger_server::exit_server()
{
	shutdown(server_socket, SHUT_RDWR);
	close(server_socket);
	printf("\n mychat is shutdown! \n *********************** \n");
}

char *messenger_server::get_local_ip()
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

void messenger_server::print_local_ip()
{
	printf("server construct init successful.\n Server IP: %s\n", (this->local_ip).c_str());
}

void messenger_server::read_user_filename(const char *fname)
{
	ifstream fin(fname, ios::in);
	if (!fin){
		fprintf(stderr, "Line: %d. Open %s failed!\n", __LINE__, fname);
		exit(1);
	}
	string line;
	while(getline(fin,line))
	{
		vector<string> *tokens = split_string(line, "|");	//user1|password1|user2;user5;user6
		if(tokens->size() > 3 || tokens->size() < 2){
			fprintf(stderr, "Line: %d. Line: %d split error!\n", __LINE__, line.c_str());
			continue;
		}
		string user = string((*tokens)[0]);
		#ifdef DEBUG
			//printf("DEBUG: string *tokens[0]: %s; *tokens[0]: %s\n",user,tokens[1]);
		#endif
		this->user_pwd.insert(pair<string, string>(user, (*tokens)[1]));
		vector<string> vc;
		if(tokens->size() == 3){	//get third part, contact information
			vector<string> *tokens2 = split_string((*tokens)[2], ";");
			for(int i = 0; i < tokens2->size(); ++i){
				vc.push_back((*tokens2)[i]);
			}
			delete tokens2;
		}
		this->user_contact.insert(pair<string, vector<string> >(user,vc));
		delete tokens;
	}
	fin.close();
}

void messenger_server::read_config_filename(const char *fname)
{
	ifstream fin(fname, ios::in);
	if (!fin){
		fprintf(stderr, "Line: %d. Open %s failed!\n", __LINE__, fname);
		exit(1);
	}

	string line;
	while(getline(fin, line)){	//keyword:value   port:5570
		vector<string> *tokens = split_string(line, ":");
		if(tokens->size() != 2){
			fprintf(stderr, "Line: %d. Line split error: %s\n", __LINE__, line.c_str());
			continue;
		}
		#ifdef DEBUG
			//printf("DEBUG: *tokens[0]:%s; *tokens[1]: %s\n",(*tokens)[0], (tokens[1]).c_str());
		#endif
		this->confile.insert(pair<string, string>((*tokens)[0], (*tokens)[1]));
		this->server_port = atoi((*tokens)[1].c_str());
		delete tokens;
	}
}

int messenger_server::init_socket()
{
	//create socket
	this->server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(this->server_socket == -1){
		//fprintf(stderr, "Line: %d. Create socket failed.\n", __LINE__);
		//return -1;
		perror("socket failed\n");
		exit(EXIT_FAILURE);
	}
	// Forcefully attaching socket to the port 8080
	int opt = 1; 
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 	//agree use local IP and port, 
	{ 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	}
	bzero((void*)&(this->server_address), sizeof(this->server_address));
	this->server_address.sin_family = AF_INET; 	//set up address family IPv4
	this->server_address.sin_addr.s_addr = INADDR_ANY; 	//set up IP address
	this->server_address.sin_port = htons(server_port); 	//set up address port
	
	//check bind
	if(bind(this->server_socket, (struct sockaddr*)&(this->server_address), sizeof(this->server_address)) == -1){
		fprintf(stderr, "Line: %d. Create socket failed.\n", __LINE__);
		return -1;
	}
	//after bind success
	fprintf(stderr, "Server IP: %s\n", inet_ntoa(this->server_address.sin_addr));
	fprintf(stderr, "Server Port: %d\n", ntohs(this->server_address.sin_port));
	//listen to clients
	if(listen(this->server_socket, MAX_U2S) == -1){
		fprintf(stderr, "Line: %d. Listen socket failed\n", __LINE__);
		return -1;
	}
	fprintf(stderr, "Line: %d. Init socket successed!\n", __LINE__);
	return 0;
}

void messenger_server::loop()
{
	struct sockaddr_in cli_addr;
	int cli_sockfd;
	socklen_t cli_sock_len;
	for(; ;)
	{
		//accept client request
		cli_sock_len = sizeof(cli_addr); //size of address
		cli_sockfd = accept(this->server_socket, (struct sockaddr *)&cli_addr, &cli_sock_len);
		fprintf(stderr, "Line: %d. Accept socket from fd: %d, ip: %s\n", __LINE__, cli_sockfd, inet_ntoa(cli_addr.sin_addr));
		if(cli_sockfd == -1)
		{
			fprintf(stderr, "Line: %d. Accept socket failed!\n", __LINE__);
			continue;
		}
		//creat pthread
		if(num_thread >= MAX_THREAD_NUM)
		{
			fprintf(stderr, "Line: %d. Exceed pthread number limitation!\n", __LINE__);
		}
		pthread_arg pthread_arg;
		pthread_arg.server = this;
		pthread_arg.socket = &cli_sockfd;
		int flag;
		flag = pthread_create(&pthread_id[num_thread], NULL, pthread_fun, &pthread_arg);
		
		if(flag != 0)
		{
			fprintf(stderr, "Line: %d. Pthread_create failed!\n", __LINE__);
			close(cli_sockfd);
		}
		++num_thread;
		fprintf(stderr, "Line: %d. _i_thread: %d\n", __LINE__, num_thread);
	}
	close(this->server_socket);
}



cmd_type messenger_server::parse_cmd(const char *line)
{
	if(strncmp(line, "CMD_REGISTER", strlen("CMD_REGISTER")) == 0){
		return CMD_REGISTER;
	}
	if(strncmp(line, "CMD_LOGIN", strlen("CMD_LOGIN")) == 0){
		return CMD_LOGIN;
	}
	if(strncmp(line, "CMD_LOGOUT", strlen("CMD_LOGOUT")) == 0){
		return CMD_LOGOUT;
	}
	if(strncmp(line, "CMD_INVITE", strlen("CMD_INVITE")) == 0){
		return CMD_INVITE;
	}
	if(strncmp(line, "CMD_ACCEPT", strlen("CMD_ACCEPT")) == 0){
		return CMD_ACCEPT;
	}
	if(strncmp(line, "CMD_REQUEST_FRIEND_ID", strlen("CMD_REQUEST_FRIEND_ID")) == 0){
		return CMD_REQUEST_FRIEND_ID;
	}
	if(strncmp(line, "CMD_EXIT", strlen("CMD_EXIT")) == 0){
		return CMD_EXIT;
	}
	if(strncmp(line, "CMD_ENTER_ROOM", strlen("CMD_ENTER_ROOM")) == 0){
                return CMD_ENTER_ROOM;
        }
	if(strncmp(line, "CMD_MSG_ROOM", strlen("CMD_MSG_ROOM")) == 0){
                return CMD_MSG_ROOM;
        }
	if(strncmp(line, "CMD_UPLOAD", strlen("CMD_UPLOAD")) == 0){
                return CMD_UPLOAD;
        }
	if(strncmp(line, "CMD_DOWNLOAD", strlen("CMD_DOWNLOAD")) == 0){
                return CMD_DOWNLOAD;
        }
	if(strncmp(line, "QUIT_ROOM", strlen("QUIT_ROOM")) == 0){
                return QUIT_ROOM;
        }
	if(strncmp(line, "CMD_REQUEST_ROOMINFO", strlen("CMD_REQUEST_ROOMINFO")) == 0){
                return CMD_REQUEST_ROOMINFO;
        }
	if(strncmp(line, "CMD_M_OUT", strlen("CMD_M_OUT")) == 0){
                return CMD_M_OUT;
        }
	if(strncmp(line, "CMD_EDIT_DONE", strlen("CMD_EDIT_DONE")) == 0){
                return CMD_EDIT_DONE;
        }

	return CMD_UNKNOWN;
}

void messenger_server::user_register(const char *line, int socket)
{
	char *user_cmd, *user_name, *user_pwd, str[strlen(line) + 1];
	memcpy(str, line, strlen(line));
	user_cmd = strtok(str, "|");
	user_name = strtok(NULL, "|");
	user_pwd = strtok(NULL, "|");
	fprintf(stderr, "Line: %d. client [%s] with pwd [%s] is registering. \n", __LINE__, user_name, user_pwd);

	//check if it registered
	int flag = 0;
	map<string, string>::iterator it = this->user_pwd.begin();
	for( ; it != this->user_pwd.end(); ++it)
	{
		if(it->first == string(user_name))
			flag = 1;
	}
	if(flag)
	{
		send(socket, "CMD_FAILED", strlen("CMD_FAILED"), 0);
		fprintf(stderr, "[%s] is already registed.\n", user_name);
		return;
	}
	add_user_pwd(user_name, user_pwd);
	add_user_contact(user_name);
	send(socket, "CMD_REGISTER_SUCCESSED", strlen("CMD_REGISTER_SUCCESSED"), 0);
	fprintf(stderr, "Line: %d. client [%s] with pwd [%s] registed successed. \n", __LINE__, user_name, user_pwd);
	
}

void messenger_server::user_login(const char *line, int socket)
{
	int i = 0;
	char *user_cmd, *user_name, *user_pwd, *user_addr, str[strlen(line) + 1], msg_buf[MSG_LEN];
	memcpy(str, line, strlen(line));
	user_cmd = strtok(str, "|");
	user_name = strtok(NULL, "|");
	user_pwd = strtok(NULL, "|");
	user_addr = strtok(NULL, "|");
	fprintf(stderr, "Line: %d. client [%s] with pwd [%s] want to login. \n", __LINE__, user_name, user_pwd);

	//check pwd if it is correct 
	int pwd_flag = -1;
	map<string, string>::iterator it = this->user_pwd.begin();
	for(	; it != this->user_pwd.end(); ++it)
	{
		fprintf(stderr, "Line: %d. user_name and pwd: [%s] : [%s]\n", __LINE__, it->first.c_str(), it->second.c_str());
		if(it->first == string(user_name))
		{
			if(it->second == string(user_pwd)){
				pwd_flag=0;
			}
		}
	}
	
	if(pwd_flag)
	{
		fprintf(stderr, "client [%s] with pwd [%s] login failed! Password error. Please re-enter.\n", user_name, user_pwd);
		send(socket, "CMD_PWDERR", strlen("CMD_PWDERR"), 0);
		return;
	}
	
	//check the online friend
	int flag = 0;
	for(int j = 0; j < (this->login_list).size(); ++j)
	{
		if((this->login_list)[j].name == string(user_name))
			flag = 1;
	}
	if(flag)
	{
		fprintf(stderr, "client [%s] with pwd  [%s] login failed! Online now.\n", user_name, user_pwd);
		send(socket, "CMD_NOWONLINE|", strlen("CMD_NOWONLINE"), 0);
		return;
	}
	fprintf(stderr, "Line: %d. Waiting for adding client [%s] to login list...\n", __LINE__, user_name);
	add2_login_list(user_name, user_pwd, user_addr, socket);
	send(socket, "CMD_LOGIN_SUCCESSED", strlen("CMD_LOGIN_SUCCESSED"), 0);
	this->total_login_users += 1;
	fprintf(stderr, "Line: %d. client [%s] login successed.\n", __LINE__, user_name);
	
	//send login msg to friends
	vector<string> friends = get_friends_info(user_name);
	memset(msg_buf, 0, sizeof(msg_buf));
	sprintf(msg_buf, "CMD_FD_ONLINE|%s|%s%c", user_name, user_addr, '\0');
	for(i = 0; i < friends.size(); ++i)
	{
		messenger_user *f_info = get_user_info(friends[i].c_str());
		if(f_info != NULL)
		{
			send(f_info->socket, msg_buf, strlen(msg_buf), 0);
			delete f_info;
		}
	}
}

void messenger_server::user_logout(const char *line, int socket)
{
	int i = 0;
	char *user_cmd, *user_name, str[MAX_CMD_LEN], msg_buf[MSG_LEN];
	memset(str, 0, MAX_CMD_LEN);
	if(line == NULL)
	{
		messenger_user *user_info = get_user_info(socket);
		if(user_info != NULL)
		{
			memcpy(str, user_info->name.c_str(), user_info->name.size());
			user_name = str;
			delete user_info;
		}
		else
		{
			fprintf(stderr, "Line: %d. [%d] logout successed. But send offline info failed!\n", __LINE__, socket);
			return;
		}
	}
	else
	{
		memcpy(str, line, strlen(line));
		user_cmd = strtok(str, "|");
		user_name = strtok(NULL,"|");
	}
	
	//check online user list
	int flag = 0;
	for(int j = 0; j < (this->login_list).size(); ++j)
	{
		if((this->login_list)[j].name == string(user_name))
			flag = 1;
	}
	if(!flag)
	{
		return;
	}
	fprintf(stderr, "Line: %d. [%s] want to logout. \n", __LINE__, user_name);
	remove_from_login_list(user_name);

	//send offline friend to friend
	vector<string> friends = get_friends_info(user_name);
	memset(msg_buf, 0, sizeof(msg_buf));
	sprintf(msg_buf, "CMD_FD_OFFLINE|%s|", user_name);
	fprintf(stderr, "Line: %d. Print friends list now, list size: %d.\n", __LINE__, friends.size());
	for(i = 0; i < friends.size(); ++i)
	{
		fprintf(stderr, "Line: %d. Check [%s]'s friend [%s] if is online.\n", __LINE__, user_name, friends[i].c_str());
		messenger_user *f_info = get_user_info(friends[i].c_str());
		if(f_info != NULL)
		{
			fprintf(stderr, "Line: %d. Send the offline info of [%s]'s to his friend [%s].\n", __LINE__, user_name, friends[i].c_str());
			send(f_info->socket, msg_buf, strlen(msg_buf), 0);
			delete f_info;
		}
	}
	fprintf(stderr, "Line: %d. [%s] logout successed. \n", __LINE__, user_name);
	return;
}

void messenger_server::user_invite(const char *line, int socket)
{
	char *user_cmd, *user_name, *user_addr, *invited_name, *invited_msg, str[strlen(line) + 1], msg_buf[MSG_LEN];
	memcpy(str, line, strlen(line));
	fprintf(stderr, "Line: %d. User cmd: [%s]\n", __LINE__, str);
	user_cmd = strtok(str, "|");
	user_name = strtok(NULL, "|");
	user_addr = strtok(NULL, "|");
	invited_name = strtok(NULL, "|");
	invited_msg = strtok(NULL, "|");
	fprintf(stderr, "Line: %d. [%s] want to invite [%s]. \n", __LINE__, user_name, invited_name);	

	messenger_user *recv_user_info = get_user_info(invited_name);
	if(recv_user_info == NULL)
	{
		memset(msg_buf, 0, sizeof(msg_buf));
		sprintf(msg_buf, "CMD_S2CMSG|The user:[%s] you invited is not online!", invited_name);
		send(socket, msg_buf, strlen(msg_buf), 0);
		fprintf(stderr, "Line: %d. [%s] invite failed! [%s] is not online!\n", __LINE__, user_name, invited_name);
		return;
	}
	memset(msg_buf, 0, sizeof(msg_buf));
	sprintf(msg_buf, "CMD_INVITE|%s|%s|%s", user_name, user_addr, invited_msg);
	send(recv_user_info->socket, msg_buf, strlen(msg_buf), 0);
	if(recv_user_info != NULL)
	{
		delete recv_user_info;
	}
	fprintf(stderr, "Line: %d. [%s] invite [%s] successed!\n", __LINE__, user_name, invited_name);
}

void messenger_server::user_accept(const char *line, int socket)
{
	char *user_cmd, *user_name, *user_addr, *inviter_name, *inviter_msg, str[strlen(line) + 1], msg_buf[MSG_LEN];
	memcpy(str, line, strlen(line));
	user_cmd = strtok(str, "|");
	user_name = strtok(NULL, "|");
	user_addr = strtok(NULL, "|");
	inviter_name = strtok(NULL, "|");
	inviter_msg = strtok(NULL, "|");
	fprintf(stderr, "Line: %d. [%s] want to accept the invitation from [%s]. \n", __LINE__, user_name, inviter_name);

	#ifdef DEBUG
		printf("DEBUG: server user_accept add friendship of : %s and %s.\n", user_name, inviter_name);
	#endif

	add_friend(user_name, inviter_name);
	add_friend(inviter_name, user_name);
	messenger_user *inviter_info = get_user_info(inviter_name);
	if(inviter_info != NULL)
	{
		#ifdef DEBUG
			printf("DEBUG:get inviter_info success, socket:%d\n", inviter_info->socket);
		#endif
		memset(msg_buf, 0, sizeof(msg_buf));
		sprintf(msg_buf, "CMD_ACCEPT|%s|%s|%s|", user_name, user_addr, inviter_msg);
		send(inviter_info->socket, msg_buf, strlen(msg_buf), 0);
		printf("Send %s to inviter user.\n", msg_buf);

		memset(msg_buf, 0, sizeof(msg_buf));
		sprintf(msg_buf, "CMD_FD_ONLINE|%s|%s|", user_name, user_addr);
		send(inviter_info->socket, msg_buf, strlen(msg_buf), 0);
		printf("Send %s to inviter user.\n",msg_buf);
		
		fprintf(stderr, "Line: %d. [%s] accept [%s]'s invitation successed!\n", __LINE__, user_name, inviter_name);
	}
	else
	{
		fprintf(stderr, "Line: %d. [%s] accept [%s]'s invitation successed! But [%s] is not online!\n", __LINE__, user_name, inviter_name, inviter_name);
	}
	memset(msg_buf, 0, sizeof(msg_buf));
	sprintf(msg_buf, "CMD_S2CMSG|Server comfirmed, you accepted [%s]'s invitation, you are friend now!", inviter_name);
	send(socket, msg_buf, strlen(msg_buf), 0);
	printf("Send %s to socket user.\n",msg_buf);

	if(inviter_info != NULL)
	{
		memset(msg_buf, 0, sizeof(msg_buf));
		sprintf(msg_buf, "CMD_FD_ONLINE|%s|%s|", inviter_info->name.c_str(), inviter_info->addr.c_str());
		send(socket, msg_buf, strlen(msg_buf), 0);
		printf("Send %s to socket user.\n",msg_buf);
	}
	if(inviter_info != NULL)
	{
		delete inviter_info;
	}
}

void messenger_server::user_friend(const char *line, int socket)
{
	int i = 0;
	char *user_cmd, *user_name, str[strlen(line) + 1], msg_buf[MSG_LEN];
	string frds_buf = "CMD_FD_OFFLINE|";	
	memcpy(str, line, strlen(line));
	user_cmd = strtok(str, "|");
	user_name = strtok(NULL, "|");

	//send friend list info to current client
	vector<string> friends = get_friends_info(user_name);
	fprintf(stderr, "Line: %d. [%s] get friend list, client has [%d] number of friends.\n", __LINE__, user_name, friends.size());
	for(i = 0; i < friends.size(); ++i)
	{
		memset(msg_buf, 0, sizeof(msg_buf));
		messenger_user *f_info = get_user_info(friends[i].c_str());
		if(f_info != NULL)
		{
			sprintf(msg_buf, "CMD_FD_ONLINE|%s|%s|%c|", f_info->name.c_str(), f_info->addr.c_str(), '\0');
			send(socket, msg_buf, strlen(msg_buf), 0);
			fprintf(stderr, "Line: %d. Send CMD_FD_ONLINE [%s] to [%s]\n", __LINE__, f_info->name.c_str(), user_name);
			delete f_info;
		}
		else
		{
			//offline here
			char tmp[MSG_LEN];
			sprintf(tmp, "%s|", friends[i].c_str());
			frds_buf += tmp;
		}
	}
	sleep(1);
	fprintf(stderr, "Line: %d. frds_buf: [%s]\n", __LINE__, frds_buf.c_str());
	send(socket, frds_buf.c_str(), strlen(frds_buf.c_str()), 0);
	fprintf(stderr, "Line: %d. Send CMD_FD_OFFLINE [%s] finished.\n", __LINE__, frds_buf.c_str());
}

void messenger_server::user_exit(const char *line, int socket)
{
	char *user_cmd, *user_name, str[strlen(line) + 1], msg_buf[MSG_LEN];
	memcpy(str, line, strlen(line));
	user_cmd = strtok(str, "|");
	user_name = strtok(NULL, "|");

	//logout 
	messenger_user *info = get_user_info(socket);
	if(info != NULL)
	{
		char buf[MSG_LEN];
		sprintf(buf, "CMD_LOGOUT|%s|\n", info->name.c_str());
		user_logout(buf, socket);
	}

	remove_from_login_list(socket);

	shutdown(socket, SHUT_RDWR);
	close(socket);
	fprintf(stderr, "Line: %d. Close socket [%d] from [%s].\n", __LINE__, socket, user_name);
	
}

void messenger_server::add2_login_list(const char *user_name, const char *user_pwd, const char *user_addr, int socket)
{
	fprintf(stderr, "Line: %d. Add [%s] [%s] [%s] [%d] to login list.\n", __LINE__, user_name, user_pwd, user_addr, socket);
	messenger_user user;
	user.socket = socket;
	user.name = string(user_name);
	user.pwd = string(user_pwd);
	user.addr = string(user_addr);
	this->login_list.push_back(user);
	fprintf(stderr, "Line: %d. Add [%s] to login list successed.\n", __LINE__, user_name);

}

messenger_user *messenger_server::get_user_info(int socket)
{
	vector<messenger_user>::iterator it = this->login_list.begin();
	for(; it != this->login_list.end(); ++it)
	{
		if(it->socket == socket){
			messenger_user *return_val = new messenger_user(*it);
			return return_val;
		}
	}
	return NULL;
}

messenger_user *messenger_server::get_user_info(const char *name)
{        
	vector<messenger_user>::iterator it = this->login_list.begin();
	for(; it != this->login_list.end(); ++it)
	{
		if(it->name == string(name))
		{
			messenger_user *return_val = new messenger_user(*it);
			return return_val;
		}
	}
        return NULL;
}


vector<string> messenger_server::get_friends_info(const char *name)
{
	vector<string> return_val;
	map<string, vector<string> >::iterator it = user_contact.begin();
	//vector<string>::iterator vc_it;
	for( ; it != user_contact.end(); ++it)
	{
		if(it->first == string(name))
		{
			return_val = it->second;
			return return_val;
		}
	}
	return return_val;
}



void messenger_server::add_user_pwd(const char *user_name, const char *user_pwd)
{
	this->user_pwd.insert(pair<string, string>(string(user_name), string(user_pwd)));
}

void messenger_server::add_user_contact(const char *user_name)
{
	vector<string> tmp;
	this->user_contact.insert(pair<string, vector<string> >(string(user_name), tmp));
}

void messenger_server::add_friend(const char *name, const char *fname)
{
	int flag = 0;
	map<string, vector<string> >::iterator it = user_contact.begin();
	vector<string>::iterator vc_it;
	for( ; it != user_contact.end(); ++it)
	{
		if(it->first == string(name))
		{
			for(vc_it = it->second.begin(); vc_it != it->second.end(); ++vc_it)
			{
				if((*vc_it) == string(fname))
				{
					flag = 1;
					break;
				}
			}
			if(!flag)
			{
				it->second.push_back(string(fname));
			}
			break;
		}
	}
}

//void messenger_server::user_logout(const char *buf, int socket)
//{
//
//}

void messenger_server::remove_from_login_list(int socket)
{
	vector<messenger_user>::iterator it = this->login_list.begin();
	for( ; it != this->login_list.end(); ++it)
	{
		if(it->socket == socket)
		{
			this->login_list.erase(it);
			break;
		}
	}
}

void messenger_server::remove_from_login_list(const char *user_name)
{
	vector<messenger_user>::iterator it = this->login_list.begin();
	for (; it != this->login_list.end(); ++it)
	{
		if (strcmp((*it).name.c_str(), user_name) == 0)
		{
			this->login_list.erase(it);
			break;
		}
	}
}
