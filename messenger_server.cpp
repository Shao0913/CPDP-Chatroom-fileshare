#include "messenger_server.h"
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>


messenger_server *ptr_this;

void sig_int_handler(int sig_num)
{
	ptr_this->exit_server();
	exit(0);
}

vector<string>* split_string(string s, const char *dev)
{
        vector<string> *vs = new vector<string>();
        char *c =  const_cast<char*>(s.c_str());        //const_cast is used to cast away the constness of variables
        char *token = std::strtok(c, dev);
        while(token != NULL)
        {
                vs->push_back(token);
                token = std::strtok(NULL, dev);
        }
        return vs;
}


void *pthread_fun(void *arg)
{
	pthread_arg *p_arg = (pthread_arg*)arg;
	messenger_server *server_arg = p_arg->server;
	int cli_socket = *(p_arg->socket);
	char recv_buf[MSG_LEN];
	socklen_t recv_length;
	for( ; ; )
	{
		recv_length = recv(cli_socket, recv_buf, MSG_LEN,0);
		if(recv_length > 0)
		{
			if(strlen(recv_buf) > 0)
				fprintf(stderr, "Line: %d. Received a msg from [%d]: [%s]\n", __LINE__, cli_socket, recv_buf);
			else
				fprintf(stderr, "Line: %d. Received a msg from [%d]: [UNKNOWN MSG]\n", __LINE__, cli_socket);
			
			switch(server_arg->parse_cmd(recv_buf))
			{
				case CMD_REGISTER:
					server_arg->user_register(recv_buf, cli_socket);
					break;
				case CMD_LOGIN:
					server_arg->user_login(recv_buf, cli_socket);
					break;
				case CMD_LOGOUT:
					server_arg->user_logout(recv_buf, cli_socket);
					break;
				case CMD_INVITE:
					server_arg->user_invite(recv_buf, cli_socket);
					break;
				case CMD_ACCEPT:
					server_arg->user_accept(recv_buf, cli_socket);
					break;
				case CMD_REQUEST_FRIEND_ID:
					server_arg->user_friend(recv_buf, cli_socket);
					break;
				case CMD_REQUEST_ROOMINFO:
					server_arg->user_roominfo(recv_buf, cli_socket);
					break;
				case CMD_ENTER_ROOM:
					server_arg->user_enter_room(recv_buf, cli_socket);
					break;
				case CMD_MSG_ROOM:
					server_arg->user_msg_room(recv_buf, cli_socket);
					break;
				case CMD_UPLOAD:
					server_arg->user_upload(recv_buf, cli_socket);
					break;
				case CMD_DOWNLOAD:
					server_arg->user_download(recv_buf, cli_socket);
					break;
				case CMD_EDIT_DONE:
					server_arg->user_edit_done(recv_buf, cli_socket);
					break;
				case CMD_M_OUT:
					server_arg->user_quit_room(recv_buf, cli_socket);
					break;
				case CMD_EXIT:
					server_arg->user_exit(recv_buf, cli_socket);
					break;
					--(server_arg->num_thread);
					return NULL;
				default:
					fprintf(stderr, "Line: %d. unknown cmd_type: %s\n", __LINE__, recv_buf);
			}
			memset(recv_buf, 0, sizeof(recv_buf));
		}
		else if(recv_length == 0)
		{
			fprintf(stderr, "Line: %d Client [%d] has exit. \n", __LINE__, cli_socket);
			server_arg->user_exit("CMD_EXIT||", cli_socket);
			--(server_arg->num_thread);
			return NULL;
		}
		else
		{
			fprintf(stderr,"Line: %d, receive msg error. \n", __LINE__);
		}
	}
	--(server_arg->num_thread);
	return NULL;
}


void messenger_server::read_user_chatroom(const char *fname)
{
	ifstream fin(fname, ios::in);
        if (!fin)
	{
                fprintf(stderr, "Line: %d. Open %s failed!\n", __LINE__, fname);
                exit(1);
        }
        string line;
        while(getline(fin,line))
        {
                vector<string> *tokens = split_string(line, "|");       //user1|password1|user2;user5;user6
                if(tokens->size() > 3 || tokens->size() < 2)
		{
                        fprintf(stderr, "Line: %d. Line: %d split error!\n", __LINE__, line.c_str());
                        continue;
                }
                string room = string((*tokens)[0]);
                this->room_pwd.insert(pair<string, string>(room, (*tokens)[1]));
                vector<string> vc;
                if(tokens->size() == 3)		//get third part, chat room member list
		{
                        vector<string> *tokens2 = split_string((*tokens)[2], ";");
                	for(int i = 0; i < tokens2->size(); ++i)
			{
                        	vc.push_back((*tokens2)[i]);
                        }
                        delete tokens2;
                }
                this->room_member.insert(pair<string, vector<string> >(room,vc));
                delete tokens;
      }
      fin.close();
}

void messenger_server::read_room_filelist(const char *fname)
{
	ifstream fin(fname, ios::in);
        if (!fin)
        {
                fprintf(stderr, "Line: %d. Open %s failed!\n", __LINE__, fname);
                exit(1);
        }
        string line;
        while(getline(fin,line))
        {
                vector<string> *tokens = split_string(line, "|");       //room1|password1|file1;file2;file3
                if(tokens->size() > 3 || tokens->size() < 2)
                {
                        fprintf(stderr, "Line: %d. Line: %d split error!\n", __LINE__, line.c_str());
                        continue;
		}
                string room = string((*tokens)[0]);                
                vector<string> vc;
                if(tokens->size() == 3)         //get third part, chat room member list
                {
                        vector<string> *tokens2 = split_string((*tokens)[2], ";");
                        for(int i = 0; i < tokens2->size(); ++i)
                        {
                                vc.push_back((*tokens2)[i]);
                        }
                        delete tokens2;
                }
                this->room_file.insert(pair<string, vector<string> >(room,vc));
                delete tokens;
      }
      fin.close();
}

void messenger_server::user_enter_room(const char *line, int socket)
{
	printf("DEBUG: received user_enter_room. \n");
	int i = 0;
	char *chatroom, *room_msg, *user_name, *user_cmd, *user_addr, str[strlen(line) + 1], msg_buf[MSG_LEN];
	memcpy(str, line, strlen(line));
	user_cmd = strtok(str, "|");
	chatroom = strtok(NULL, "|");
	room_msg = strtok(NULL, "|");
	user_addr = strtok(NULL, "|");
	user_name = strtok(NULL, "|");
	fprintf(stderr, "Line: %d. User [%s] chatroom [%s] with pwd(msg) [%s]. \n", __LINE__, user_name, chatroom, room_msg);

	//check pwd if it is correct
	if(room_pwd_check(chatroom, room_msg))
	{
		fprintf(stderr, "chatroom [%s] with pwd [%s] enter failed! Password error. Please re-enter.\n", chatroom, room_msg);
		send(socket, "CMD_PWDERR", strlen("CMD_PWDERR"), 0);
		return;
	}

	//check the chat room member list
	if(room_member_check(chatroom, user_name))
	{
		fprintf(stderr, "client [%s] enter chatroom [%s] with pwd  [%s] enter failed! In room now.\n", user_name,chatroom, room_msg);
		send(socket, "CMD_INROOM|", strlen("CMD_INROOM"), 0);
		return;
	}
	fprintf(stderr, "Line: %d. Waiting for adding client [%s] to chatroom [%s]...\n", __LINE__, user_name, chatroom);
	add2_room_member(chatroom, user_name);
	send(socket, "CMD_ENTER_SUCCESSED", strlen("CMD_ENTER_SUCCESSED"), 0);
	//this->total_login_users += 1;
	fprintf(stderr, "Line: %d. client [%s] enter chatroom [%s] successed.(send CMD_ENTER_SUCCESSED)\n", __LINE__, user_name, chatroom);
	
	//send enter room msg to room member
	vector<string> r_member = get_member_info(chatroom);
	memset(msg_buf, 0, sizeof(msg_buf));
	sprintf(msg_buf, "CMD_USER_ENTER|%s|%s%c", user_name, user_addr, '\0');
	for(i = 0; i < r_member.size(); ++i)
	{
		messenger_user *m_info = get_user_info(r_member[i].c_str());
		if(m_info != NULL && m_info->name != user_name)
		{
			send(m_info->socket, msg_buf, strlen(msg_buf), 0);
			delete m_info;
		}
	}
}

void messenger_server::user_msg_room(const char *line, int socket)
{
	#ifdef DEBUG
                printf("DEBUG: This is user_msg_room throught server, msg: [%s].", line);
        #endif
	int i = 0;
	char *user_cmd, *user_name, *room_msg, *room_name, str[strlen(line) + 1], msg_buf[MSG_LEN];
	memcpy(str, line, strlen(line));
	user_cmd = strtok(str, "|");
	room_name = strtok(NULL, "|");
	room_msg = strtok(NULL, "|");
	user_name = strtok(NULL, "|");

	fprintf(stderr, "User [%s] send to chatroom [%s] with msg [%s].\n", user_name,room_name, room_msg);
	
	//send enter room msg to room member
	vector<string> r_member = get_member_info(room_name);
	memset(msg_buf, 0, sizeof(msg_buf));
	sprintf(msg_buf, "CMD_MSG_ROOM|%s|%s|%s%c", user_name, room_name, room_msg, '\0');
	for(i = 0; i < r_member.size(); ++i)
	{
		messenger_user *m_info = get_user_info(r_member[i].c_str());
		if(m_info != NULL)
		{
			send(m_info->socket, msg_buf, strlen(msg_buf), 0);
			delete m_info;
		}
	}
}

void messenger_server::user_upload(const char *line, int socket)
{
	fprintf(stderr, "LINE %d. Server receive the upload cmd.\n", __LINE__);
        int i = 0;
        char *room_name, *user_name, *user_cmd, *file_name, str[strlen(line) + 1], msg_buf[MSG_LEN], recv_buf[MSG_LEN], send_buf[MSG_LEN];
        //socklen_t recv_length;
	memcpy(str, line, strlen(line));
        user_cmd = strtok(str, "|");
        room_name = strtok(NULL, "|");
        file_name = strtok(NULL, "|");
	user_name = strtok(NULL, "|");
        fprintf(stderr, "Line: %d. User [%s] want to upload file [%s] to chatroom [%s]. \n", __LINE__, user_name, file_name, room_name);

	messenger_user *m_info = get_user_info(user_name);
	if(m_info == NULL)
	{
		fprintf(stderr, "ERROR: %d. Get user_info error.\n", __LINE__, user_name);
		return;
	}

	//check the file if it exist in the room_file and start recv from client
	int flag = 0;
	int file_fp;
	int finished = 1;
	flag = check_file_status(user_name, file_name, room_name);
	int length = 0, socket_len = 0;
	if(flag)
	{
		fprintf(stderr, "Error %d. File [%s] is exist in room [%s].\n", __LINE__, file_name, room_name);
		//send upload error backe to client
	}
	else
	{
		//add file to room_list
		add_room_file(room_name, file_name);
		fprintf(stderr, "LINE %d. Upload file add to room_file list.\n", __LINE__);
		
		//open file and ready for write
		char new_file_name[FILE_NAME_LEN];
		sprintf(new_file_name, "%s_client2_%s", file_name, user_name);
		file_fp = open(new_file_name,O_CREAT|O_RDWR,0777); 
     		if(file_fp < 0) 
     		{ 
         		printf("File:\t%s Can Not Open To Write\n", new_file_name); 
         		exit(1); 
     		}

		//enter while to recv content of file		
		/*memset(recv_buf, 0, sizeof(recv_buf));
		recv_length = recv(cli_socket, recv_buf, MSG_LEN,0);
		if(recv_length > 0)
		{
			if(strlen(recv_buf) > 0)
				fprintf(stderr, "Line: %d. Received a msg from [%d]: [%s]\n", __LINE__, cli_socket, recv_buf);
			else
				fprintf(stderr, "Line: %d. Received a msg from [%d]: [UNKNOWN MSG]\n", __LINE__, cli_socket);
		}*/
		while(finished)
		{
			memset(recv_buf, 0, sizeof(recv_buf));
			socket_len=0;
                	socket_len = recv(socket, recv_buf, MSG_LEN,0);
                	if(socket_len > 0)
                	{
                        	if(strlen(recv_buf) > 0)
                        	        fprintf(stderr, "Line: %d. Received a msg from [%d]: [%s]\n", __LINE__, socket, recv_buf);
                	        else
        	                        fprintf(stderr, "Line: %d. Received a msg from [%d]: [UNKNOWN MSG]\n", __LINE__, socket);
	                }

			#ifdef DEBUG
                        	printf("DEBUG:Receive recv_buf: [%s].\n", recv_buf);
                        #endif
			if(strncmp(recv_buf, "CMD_UPLOAD_FINISHED", strlen("CMD_UPLOAD_FINISHED")) == 0)
			{
				//received the finishe msg
				#ifdef DEBUG
                                        printf("DEBUG:Receive upload finished msg, and break while loop: [%s].\n", recv_buf);
                                #endif
				finished = 0;	//break the while loop
				break;
			}
			else
			{ 
				//write into file
				#ifdef DEBUG
                                        printf("DEBUG:Receive recv_buf: [%s].\n", recv_buf);
                                #endif
				if(write(file_fp, recv_buf, socket_len) < socket_len)
				{
					fprintf(stderr, "\nLine: %d. Write recv_buf to file [%s] failed!\n", __LINE__, new_file_name);
					break;
				}
				else
				{
					fprintf(stderr, "\nLine: %d. Write recv_buf to file [%s] successed!\n", __LINE__, new_file_name);
				}
				memset(recv_buf, 0, sizeof(recv_buf));	
			}
		}
		fprintf(stderr, "\nLine: %d. Write to file [%s] finished and close file_fp!\n", __LINE__, new_file_name);
		close(file_fp);
	}

	
}

void messenger_server::user_download(const char *line, int socket)
{
	fprintf(stderr, "LINE %d. Server receive download request [%s].\n", __LINE__, line);
        int i = 0;
        char *room_name, *user_name, *user_cmd, *file_name, str[strlen(line) + 1], msg_buf[MSG_LEN];
        memcpy(str, line, strlen(line));
        user_cmd = strtok(str, "|");
        room_name = strtok(NULL, "|");
	file_name = strtok(NULL, "|");
	user_name = strtok(NULL, "|");
        fprintf(stderr, "Line: %d. User [%s] ask download file [%s] from chatroom [%s]. \n", __LINE__, user_name, file_name, room_name);

	messenger_user *m_info = get_user_info(user_name);
	if(m_info == NULL)
	{
		fprintf(stderr, "ERROR: %d. Get user_info error.\n", __LINE__, user_name);
		return;
	}

	int flag = 0;
	int finished = 1;
	flag = check_file_status(user_name, file_name, room_name);
	int length = 0, socket_len = 0;
	if(flag)
	{
		#ifdef DEBUG
			printf("DEBUG: The file is in the chatroom.\n");
		#endif
		//open file and ready to send
		int file_fp;
        	file_fp = open(file_name, O_RDONLY, 0777);
        	if(file_fp >= 0 )
        	{
        	        fprintf(stderr, "Line: %d. Open file [%s] on server successed. \n", __LINE__, file_name);
        	}
        	else
        	{
                	memset(msg_buf, 0, sizeof(msg_buf));
                        sprintf(msg_buf, "CMD_FILE_ERROR|%c", '\0');
			#ifdef DEBUG
				printf("DEBUG:open file failed, sending CMD_FILE_ERROR.\n");
			#endif
			send(m_info->socket, msg_buf, strlen(msg_buf), 0);
                	return;
        	}
		
		//file exist or file is in this room, start transport
		char send_buf[MSG_LEN+20];
		memset(send_buf, 0, sizeof(send_buf));
		memset(msg_buf, 0, sizeof(msg_buf));
		while((length = read(file_fp, msg_buf, sizeof(msg_buf))) > 0)
		{
			#ifdef DEBUG
				printf("DEBUG: sending file content: [%s]\n", msg_buf);
			#endif
			sprintf(send_buf, "CMD_D_CONTENT|%s%c", msg_buf);
			send(m_info->socket, msg_buf, strlen(msg_buf), 0);
			memset(send_buf, 0, sizeof(send_buf));
			memset(msg_buf, 0, sizeof(msg_buf));
			sleep(1);
		}
		memset(msg_buf, 0, sizeof(msg_buf));                
		#ifdef DEBUG
			printf("DEBUG:sending finished, sending CMD_DOWNLOAD_FINISHED.\n");
		#endif
		sprintf(msg_buf, "CMD_DOWNLOAD_FINISHED|%c", '\0');
		send(m_info->socket, msg_buf, strlen(msg_buf), 0);
		close(file_fp);
	}
	else
	{
		//file is not exist or something error, send error cmd
		memset(msg_buf, 0, sizeof(msg_buf));
		sprintf(msg_buf, "CMD_FILE_ERROR|%c", '\0');
		#ifdef DEBUG
			printf("DEBUG: file is not exist in file_list, sending CMD_FILE_ERROR.\n");
		#endif
		send(m_info->socket, msg_buf, strlen(msg_buf), 0);
	}

}

void messenger_server::user_quit_room(const char *line, int socket)
{
	fprintf(stderr, "LINE %d. User quit from room need to update to everyone in room.\n", __LINE__);
	int i = 0;
	char *room_name, *user_name, *user_cmd, str[strlen(line) + 1], msg_buf[MSG_LEN];
	memcpy(str, line, strlen(line));
	user_cmd = strtok(str, "|");
	user_name = strtok(NULL, "|");
	room_name = strtok(NULL, "|");
	fprintf(stderr, "Line: %d. User [%s] exiting chatroom [%s]. \n", __LINE__, user_name, room_name);
	
	remove_room_member(user_name, room_name);
	fprintf(stderr, "Line: %d. client [%s] removed from chatroom [%s] successed.\n", __LINE__, user_name, room_name);
	
	//send out member to the rest of room member
	vector<string> r_member = get_member_info(room_name);
	memset(msg_buf, 0, sizeof(msg_buf));
	sprintf(msg_buf, "CMD_M_OUT|%s|%s%c", user_name, room_name, '\0');
	for(i = 0; i < r_member.size(); ++i)
	{
		messenger_user *m_info = get_user_info(r_member[i].c_str());
		if(m_info != NULL && m_info->name != user_name)
		{
			send(m_info->socket, msg_buf, strlen(msg_buf), 0);
			delete m_info;
		}
	}
}

void messenger_server::user_roominfo(const char *line, int socket)
{
	int i = 0;
	char *chatroom, *user_cmd, *user_name, str[strlen(line) + 1], msg_buf[MSG_LEN];
	string m_buf = "CMD_M_OUT|";	
	memcpy(str, line, strlen(line));
	user_cmd = strtok(str, "|");
	user_name = strtok(NULL, "|");
	chatroom = strtok(NULL, "|");

	//send chat room list to current client
	fprintf(stderr, "Line: %d. [%s] get room_list, there are [%d] number of rooms.\n", __LINE__, user_name, (this->room_pwd).size());
	map<string, string>::iterator it=this->room_pwd.begin();
	for( ; it != this->room_pwd.end(); ++it)
	{
		memset(msg_buf, 0, sizeof(msg_buf));
		sprintf(msg_buf, "CMD_ROOMLIST|%s|%c|", (it->first).c_str(), '\0');
		send(socket, msg_buf, strlen(msg_buf), 0);
                fprintf(stderr, "Line: %d. Send CMD_ROOMLIST [%s] to [%s]: msg_buf: [%s].\n", __LINE__, (it->first).c_str(), user_name, msg_buf);
		sleep(1);
	}

	
	//send chat room info(member) to current client
	vector<string> m_list = get_member_info(chatroom);
	fprintf(stderr, "Line: %d. [%s] get chatroom [%s] list, chatroom has [%d] number of users.\n", __LINE__, user_name,chatroom, m_list.size());
	for(i = 0; i < m_list.size(); ++i)
	{
		memset(msg_buf, 0, sizeof(msg_buf));
		messenger_user *f_info = get_user_info(m_list[i].c_str());
		if(f_info != NULL)
		{
			sprintf(msg_buf, "CMD_USER_ENTER|%s|%s|%c|", f_info->name.c_str(), f_info->addr.c_str(), '\0');
			send(socket, msg_buf, strlen(msg_buf), 0);
			fprintf(stderr, "Line: %d. Send CMD_USER_ENTER [%s] to [%s]\n", __LINE__, f_info->name.c_str(), user_name);
			delete f_info;
		}
		else
		{
			//offline here
			char tmp[MSG_LEN];
			sprintf(tmp, "%s|", m_list[i].c_str());
			m_buf += tmp;
		}
	}
	sleep(1);
	fprintf(stderr, "Line: %d. m_buf: [%s]\n", __LINE__, m_buf.c_str());
	send(socket, m_buf.c_str(), strlen(m_buf.c_str()), 0);
	fprintf(stderr, "Line: %d. Send CMD_M_OUT [%s] finished.\n", __LINE__, m_buf.c_str());

}

void messenger_server::user_edit_done(const char *line, int socket)
{
	printf("DEBUG: This is user edit done.............\n");
}


//common helper functions
int messenger_server::room_pwd_check(const char *chatroom, const char *room_msg)
{
	int pwd_flag = -1;
        map<string, string>::iterator it = this->room_pwd.begin();
        for(    ; it != this->room_pwd.end(); ++it)
        {
                fprintf(stderr, "Line: %d. room_name and pwd: [%s] : [%s]\n", __LINE__, it->first.c_str(), it->second.c_str());
                if(it->first == string(chatroom))
                {
                        if(it->second == string(room_msg)){
                                pwd_flag=0;
                        }
                }
        }
	return pwd_flag;
}

int messenger_server::room_member_check(const char *chatroom, const char *user_name)
{
	int flag = 0;
        map<string, vector<string> >::iterator it_rm = room_member.begin();
        vector<string>::iterator vc_it;
        for( ; it_rm != room_member.end(); ++it_rm)
        {
                if(it_rm->first  == string(chatroom))
                {
                        for(vc_it = it_rm->second.begin(); vc_it != it_rm->second.end(); ++vc_it)
                                if((*vc_it) == string(user_name))
                                {
                                        flag = 1;
                                        break;
                                }
                }
        }
	return flag;	
}

int messenger_server::check_file_status(const char *user_name, const char *file_name, const char *room_name)
{	
	int flag = 0;
	map<string, vector<string> >::iterator it_rf = room_file.begin();
	vector<string>::iterator vc_it;
	for( ; it_rf != room_file.end(); ++it_rf)
	{
		if(it_rf->first == string(room_name))
		{
			for(vc_it = it_rf->second.begin(); vc_it != it_rf->second.end(); ++vc_it)
			{
				if((*vc_it) == string(file_name))
				{
					flag = 1;
					break;
				}
			}
		}
	}
	return flag;
}

//helper function for user_enter_chatroom
void messenger_server::add2_room_member(const char *room_name, const char *user_name)
{
	map<string, vector<string> >::iterator it_rm = room_member.begin();
        for( ; it_rm != room_member.end(); ++it_rm)
        {
                if(it_rm->first  == string(room_name))
                {
                        it_rm->second.push_back(user_name);
                }
        }
}

vector<string> messenger_server::get_member_info(const char *room_name)
{
	vector<string>  return_val;
	map<string, vector<string> >::iterator it_rm = room_member.begin();
        for( ; it_rm != room_member.end(); ++it_rm)
        {
                if(it_rm->first  == string(room_name))
                {
                        return_val = it_rm -> second;
			return return_val;
                }
        }
	return return_val;
}

void messenger_server::add_room_file(const char *room_name, const char *fname)
{
	map<string, vector<string> >::iterator it_rf = room_file.begin();
	vector<string>::iterator vc_it;
	for( ; it_rf != room_file.end(); ++it_rf)
	{
		if(it_rf->first == string(room_name))
		{
			it_rf->second.push_back(fname);
		}
	}
}

void messenger_server::remove_room_member(const char *user_name, const char *room_name)
{
	#ifdef DEBUG
		printf("DEBUG: remove room member seg fault check: begin.\n");
	#endif
	map<string, vector<string> >::iterator it_rm = room_member.begin();
	vector<string>::iterator it_vc;
        for( ; it_rm != room_member.end(); ++it_rm)
        {
                if(it_rm->first  == string(room_name))
                {
			for(it_vc=it_rm->second.begin(); it_vc != it_rm->second.end(); ++it_vc)
			{
				if((*it_vc)==string(user_name))
				{
					(it_rm->second).erase(it_vc);
					return;
				}
			}
                }
        }
	#ifdef DEBUG
		printf("DEBUG: check, ending.\n");
	#endif
}


int main(int argc, char const *argv[])
{
	if (argc != 5){
		fprintf(stderr, "Usage: messenger_client user_info_file configuration_file config_room config_file_list\n");
		return 0;
	}

	messenger_server  *myserver = new messenger_server(argv[1], argv[2], argv[3], argv[4]);
	myserver->print_local_ip();
	int init;
	init = myserver -> init_socket();
	if(init)
	{
		fprintf(stderr, "Line: %d. Init_socket failed!\n", __LINE__);
		return 0;
	}
	ptr_this = myserver;
	signal(SIGINT, sig_int_handler);
	myserver->loop();
	
	return 0;
}

