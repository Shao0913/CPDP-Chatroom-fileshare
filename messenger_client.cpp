#include "messenger_client.h"

#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <map>
#include <vector>

//using namespace std;

using std::ifstream;
using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::map;
using std::ios;
using std::pair;

messenger_client *ptr_this;

vector<string>* split_string(string s, const char *dev)
{
	vector<string> *vs = new vector<string>();
	char *c =  const_cast<char*>(s.c_str());	//const_cast is used to cast away the constness of variables
	char *token = std::strtok(c, dev);
	while(token != NULL)
	{
		vs->push_back(token);
		token = std::strtok(NULL, dev);
	}
	return vs;
}

void sig_int_handler(int sig_num)
{
	#ifdef DEBUG
		printf("DEBUG: sig handler, exiting.\n");
	#endif
	ptr_this->user_exit();
	exit(0);
}


void *pthread_fun(void *arg)
{
	for( ; ; )
	{
		if(ptr_this->inchat_flag == 0)
		{
			ptr_this->menu_2();
		}
		string cmd_buf = ptr_this->get_input();
		#ifdef DEBUG
			printf("cmd get form get_input(): %s\n", cmd_buf.c_str());
		#endif
		if(cmd_buf.size()>0)
		{
			switch(ptr_this->parse_cmd(cmd_buf))
			{
				case CMD_ENTER_ROOM:
					ptr_this->user_enter_room(cmd_buf);
					break;
				case CMD_SENDMSG:
					ptr_this->user_sendmsg(cmd_buf);
					break;
				case CMD_INVITE:
					ptr_this->user_invite(cmd_buf);
					break;
				case CMD_ACCEPT:
					ptr_this->user_accept(cmd_buf);
					break;
				case CMD_LOGOUT:
					ptr_this->user_logout();
					ptr_this->thread_finish_flag = 1;
					return NULL;
				default:
					printf("Unknown cmd: [%s]\n", cmd_buf.c_str());
					break;
			}
		}
	}
	ptr_this->thread_finish_flag = 1;
	return NULL;
}

void *pthread_chat_fun(void *arg)
{
	for( ; ; )
        {
                ptr_this->menu_3();
                string cmd_buf = ptr_this->get_input();
                #ifdef DEBUG
                        printf("cmd get form pthread_chat_fun's get_input(): %s\n", cmd_buf.c_str());
                #endif
                if(cmd_buf.size()>0)
                {
                        switch(ptr_this->parse_cmd(cmd_buf))
                        {
                                case CMD_MSG_ROOM:
                                        ptr_this->sendmsg2_room(cmd_buf);
                                        break;
                                case CMD_UPLOAD:
                                        ptr_this->upload2_room(cmd_buf);
                                        break;
                                case CMD_DOWNLOAD:
                                        ptr_this->download4_room(cmd_buf);
                                        break;
                                case QUIT_ROOM:
                                        ptr_this->quit_room();
                                        ptr_this->thread_chat_finish_flag = 1;
                                        return NULL;
				case CMD_EDIT:
					ptr_this->edit(cmd_buf);
					break;
                                default:
                                        printf("Unknown cmd: [%s]\n", cmd_buf.c_str());
                                        break;
                        }
                }
        }
        ptr_this->thread_chat_finish_flag = 1;
        return NULL;
}

void messenger_client::user_enter_room(string cmd)
{
	int pos1, pos2, flag, f_socket;
	string chatroom, room_msg;
	pos1 = cmd.find(' ');
	pos2 = cmd.find(' ', pos1 + 1);
	chatroom = cmd.substr(pos1 + 1, pos2 - pos1 - 1);
	room_msg    = cmd.substr(pos2 + 1);

	/*messenger_user *r_list = get_room_info(chatroom);
	if(r_list == NULL)
	{
		printf("%s is not in exist.\n", chatroom.c_str());
		return;
	}
	delete r_list;*/
	int socket_len = 0;
	char recv_buf[MSG_LEN], send_buf[MSG_LEN];
	sprintf(send_buf, "CMD_ENTER_ROOM|%s|%s|%s|%s|", chatroom.c_str(), room_msg.c_str(), (this->local_ip).c_str(), (this->client_name).c_str());
        socket_len = send(this->c2s_socket, send_buf, strlen(send_buf), 0);
        if(socket_len <= 0)
        {
                fprintf(stderr, "Line: %d. Send msg failed!\n", __LINE__);
                return;
        }
	printf("Waiting for the server response...\n");
	socket_len = 0;
	socket_len = recv(this->c2s_socket, recv_buf, MSG_LEN, 0);
	if (socket_len > 0)
	{
		if(strncmp(recv_buf, "CMD_ENTER_SUCCESSED", strlen("CMD_ENTER_SUCCESSED")) == 0)
		{
			printf("DEBUG: receive server msg: %s.\n",recv_buf);
			printf("Enter chatroom [%s] successed!.\n", chatroom.c_str());
			this->inchat_flag = 1;
			this->inchat_room = chatroom;
			chat_loop(chatroom.c_str());
			return;
		}
		else
		{
			fprintf(stderr, "Line: %d, chatroom password error!\n", __LINE__);
		}
	}
	else
	{
		fprintf(stderr, "Line: %d, recv msg failed!\n", __LINE__);
	}
}

void messenger_client::chat_loop(const char *chatroom)
{
	printf("DEBUG: here enter chat loop where will disp chat room info.\n");
	int flag = 0;
	//initial chatroom info from server
	flag = init_room_info(chatroom);
	//sleep(this->room_list.size());
	//#ifdef DEBUG
        //	printf("DEBUG: Main thread sleep %d!\n",this->room_list.size());
       	//#endif
	if(flag)
	{
		fprintf(stderr, "Line: %d. Request chatroom info failed!\n", __LINE__);
		return;
	}
	else
	{
		#ifdef DEBUG
			printf("DEBUG: Request chatroom info successed!\n");
		#endif
	}

	//listen to other chat room members
	//listen init is initiated in login_loop
	
	//create a thread to handle the input cmd from user
	this->thread_chat_finish_flag = 0;
	flag = pthread_create(&pthread_chat_id, NULL, pthread_chat_fun, NULL);
	if(flag !=0)
	{
		fprintf(stderr, "Line: %d. Pthread_chat_create failed!\n", __LINE__);
		return;
	}

	//select socket from the server, new connection, and other clients
	int i = 0, new_client_fd;
	//socklen_t new_client_size;
	char recv_buf[MSG_LEN];
	//vector<int>::iterator vc_it;
	fd_set fdss,allset,origset;
	struct timeval timeout_val;
	//struct sockaddr_in new_cli_addr;
	//this->c2c_online_num = 0;
	//this->c2c_socket_vc.clear();

	while(1)
	{
		while(gobal_flag)
		{
			;
		}
		if(this->thread_chat_finish_flag || listen_socket < 0 || c2s_socket < 0)
		{
			room_clear();
			return;
		}
		
		//clear the socket set
		FD_ZERO(&allset);
		//add server to client socket
		//only need to listen to server to client socket but still use select()
		FD_SET(this->c2s_socket, &allset);
		FD_SET(this->listen_socket, &allset);
		//setup timeout value
		timeout_val.tv_sec = 0;
		timeout_val.tv_usec = 100000;

		flag = select(FD_SETSIZE, &allset, NULL, NULL, &timeout_val);
		if(flag < 0)
		{
			fprintf(stderr, "Line: %d. Select failed!\n", __LINE__);
			break;
		}
		else if (flag== 0)
		{
			//fprintf(stderr, "Line: %d.chat_loop Select timeout.\n", __LINE__);
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
			//server send to client messenger handler
			#ifdef DEBUG
				printf("DEBUG: receive msg from server: %s\n",recv_buf);
			#endif
			s2c_room_msg_handler(recv_buf);
			continue;
		}
		
		//check a new connection
		if(FD_ISSET(this->listen_socket, &allset))
		{
			fprintf(stderr, "Line: %d. Ignore listen_socket, don't accept and continue!\n", __LINE__);
			continue;
		}
	}
}

//chat loop call thread_chat handler chat cmd function
void messenger_client::sendmsg2_room(string cmd)
{
	#ifdef DEBUG
		printf("DEBUG: This is sendmsg2_room throught server, cmd: [%s].", cmd.c_str());
	#endif
	int pos1, pos2, flag, f_socket;
	string room_target, room_msg;
	pos1 = cmd.find(' ');
	pos2 = cmd.find(' ', pos1 + 1);
	room_target = cmd.substr(pos1 + 1, pos2 - pos1 - 1);
	room_msg    = cmd.substr(pos2 + 1);

	#ifdef DEBUG
		printf("DEBUG:sendmsg2_room room member size: %d", chat_member.size());
	#endif

	int socket_len = 0;
	char send_buf[MSG_LEN];
	//messenger_user *r_list = get_room_info(chatroom);
	if(chat_member.empty())
	{
		printf("%s chatroom doesn't have any member.\n", room_target.c_str());
		return;
	}
	else
	{
		sprintf(send_buf, "CMD_MSG_ROOM|%s|%s|%s|", room_target.c_str(), room_msg.c_str(), (this->client_name).c_str());
		socket_len = send(this->c2s_socket, send_buf, strlen(send_buf), 0);
		printf("Messenge [%s] send to server.\n", send_buf);
	}

	
}

void messenger_client::upload2_room(string cmd)
{
	#ifdef DEBUG
        	printf("DEBUG: This is upload2_room throught server, cmd: [%s].", cmd.c_str());
        #endif

	//disable listening
	gobal_flag = 1;
	sleep(1);

	int pos1, pos2, flag, f_socket;
        string room_target, file_name;
        pos1 = cmd.find(' ');
        pos2 = cmd.find(' ', pos1 + 1);
        room_target = cmd.substr(pos1 + 1, pos2 - pos1 - 1);
        file_name    = cmd.substr(pos2 + 1);

        #ifdef DEBUG
                printf("DEBUG:upload2_room room member size: %d\n", chat_member.size());
        #endif

        int socket_len = 0;
        char send_buf[MSG_LEN], recv_buf[MSG_LEN];
	int finished = 1;
	printf("Sending file upload request...\n");
        sprintf(send_buf, "CMD_UPLOAD|%s|%s|%s|", room_target.c_str(), file_name.c_str(), (this->client_name).c_str());
        socket_len = send(this->c2s_socket, send_buf, strlen(send_buf), 0);

	sleep(1);	//wait server enter while loop to receive
	//make file ready to upload
	int file_fp;
	int length = 0;
        file_fp = open(file_name.c_str(), O_RDONLY, 0777);
        if(file_fp >= 0 )
        {
                fprintf(stderr, "Line: %d. Open file [%s] on client successed. \n", __LINE__, file_name.c_str());
        }
        else
        {
                //memset(msg_buf, 0, sizeof(msg_buf));
                //sprintf(msg_buf, "CMD_FILE_ERROR|%c", '\0');
		printf("DEBUG:open file failed, sending CMD_FILE_ERROR.\n");
		//send(m_info->socket, msg_buf, strlen(msg_buf), 0);
               	return;
        }
	printf("Uploading....");
	while((length = read(file_fp, send_buf, sizeof(send_buf))) > 0)
	{
		printf("..");
		#ifdef DEBUG
			printf("DEBUG: sending file content: [%s]\n", send_buf);
		#endif
		send(this->c2s_socket, send_buf, strlen(send_buf), 0);
		memset(send_buf, 0, sizeof(send_buf));
		sleep(1);
		//finished = 0;
	}
	printf("Uploading finished!\n");
	//sending finished msg
	memset(send_buf, 0, sizeof(send_buf));                
	#ifdef DEBUG
		printf("DEBUG:sending finished, sending CMD_UPLOAD_FINISHED.\n");
	#endif
	sprintf(send_buf, "CMD_UPLOAD_FINISHED|%c", '\0');
	send(this->c2s_socket, send_buf, strlen(send_buf), 0);
	
	close(file_fp);
	gobal_flag = 0;	//enable listening

}

void messenger_client::download4_room(string cmd)
{
	 #ifdef DEBUG
                printf("DEBUG: This is download4_room throught server, cmd: [%s].\n", cmd.c_str());
        #endif

	//disable listen
	gobal_flag = 1;	
	sleep(1);	//sleep 1 sec to wait listen closed

	int pos1, pos2, flag, f_socket;
        string room_target, file_name;
        pos1 = cmd.find(' ');
        pos2 = cmd.find(' ', pos1 + 1);
        room_target = cmd.substr(pos1 + 1, pos2 - pos1 - 1);
        file_name    = cmd.substr(pos2 + 1);
	
        #ifdef DEBUG
                printf("DEBUG:download4_room room member size: %d\n", chat_member.size());
        #endif

        int socket_len = 0;
        char send_buf[MSG_LEN],recv_buf[MSG_LEN];
	int file_fp;
	printf("Sending file download request...\n");
	sprintf(send_buf, "CMD_DOWNLOAD|%s|%s|%s|", room_target.c_str(), file_name.c_str(), (this->client_name).c_str());
	socket_len = send(this->c2s_socket, send_buf, strlen(send_buf), 0);
	if (socket_len > 0)
	{
		printf("Messenge [%s] send to server.\n", send_buf);
		
		//open a file and ready to write
		char new_file_name[FILE_NAME_LEN];
		sprintf(new_file_name, "%s_server2_%s", file_name.c_str(), (this->client_name).c_str());
		file_fp = open(new_file_name,O_CREAT|O_RDWR,0777); 
     		if(file_fp < 0) 
     		{ 
         		printf("File:\t%s Can Not Open To Write\n", new_file_name); 
         		exit(1); 
     		}

		download_flag = 1;
		int length = 0;
		memset(recv_buf, 0, sizeof(recv_buf));
        	printf("Downloading..");
		while(download_flag)
		{	
			//sleep(1);
			printf("..");
			socket_len = 0;
                	socket_len= recv(this->c2s_socket, recv_buf, MSG_LEN, 0);
			#ifdef DEBUG
				printf("DEBUG: Receive buf is [%s]\n", recv_buf);
			#endif
			if(strncmp(recv_buf, "CMD_DOWNLOAD_FINISHED", strlen("CMD_DOWNLOAD_FINISHED")) == 0)
			{	
				//if recv this msg means file transport finished
				#ifdef DEBUG
					printf("DEBUG: receive CMD_DOWNLOAD_FINISHED.\n");
				#endif
				download_flag = 0;	//change flag to break while loop
				break;
			}
			else if(strncmp(recv_buf, "CMD_FILE_ERROR", strlen("CMD_FILE_ERROR")) == 0)
			{
				fprintf(stderr, "\nERROR: %d. Server says file [%s] has error, check your file name or other stuff!\n", __LINE__, file_name.c_str());
                        	download_flag = 0;	//break while loop
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
				memset(recv_buf, 0, sizeof(recv_buf));
			}

		}
		
        	printf("..\n Downloading finished!\n");
		close(file_fp);
		gobal_flag = 0;
	}
}

void messenger_client::edit(string cmd)
{
	int pos1, pos2, flag, f_socket;
        string room_target, file_name;
        pos1 = cmd.find(' ');
        pos2 = cmd.find(' ', pos1 + 1);
        room_target = cmd.substr(pos1 + 1, pos2 - pos1 - 1);
        file_name    = cmd.substr(pos2 + 1);

	//call download function
	printf("Downloading file [%s] from server.\n", cmd.c_str());
	download4_room(cmd);
	char new_file_name[FILE_NAME_LEN];
	sprintf(new_file_name, "%s_server2_%s", file_name.c_str(), (this->client_name).c_str());
		
	gobal_flag = 1;	//disable listen in main thread again
	//system call vim to start editting
	//char *command = "vim";
	//char **parameter = new_file_name;
	//char* command[] = {"vim", new_file_name, NULL};
	//execvp(command[0], command);

	int status, sys_flag = 1;
	pid_t child;
	while(sys_flag)
	{
		if((child=fork())==0)
		{
			char* command[] = {"vim", new_file_name, NULL};
			execvp(command[0], command);
		}
		else
		{
			waitpid(child, &status, 0);
			sys_flag = 0;
		}
	}

	
	//call upload function to upload server
	string new_cmd;
	new_cmd.append("edit ");
	new_cmd.append(room_target);
	new_cmd.append(" ");
	new_cmd.append(new_file_name);
	printf("Uploading file [%s] to server.\n", new_cmd.c_str());
	upload2_room(new_cmd);
	char changed_file_name[FILE_NAME_LEN];
        sprintf(changed_file_name, "%s_client2_%s", new_file_name, (this->client_name).c_str());

	char send_buf[MSG_LEN];
	int socket_len = 0;
	//send request msg to ask server merge file
	sprintf(send_buf, "CMD_EDIT_DONE|%s|%s|%s|", room_target.c_str(), changed_file_name, (this->client_name).c_str());
	socket_len = send(this->c2s_socket, send_buf, strlen(send_buf), 0);
	printf("Messenge [%s] send to server.\n", send_buf);
	gobal_flag = 0; 	//enable listen in main thread
}

void messenger_client::quit_room()
{
	#ifdef DEBUG
                printf("DEBUG: This is quit_room throught server.\n");
        #endif
	int flag = 0;
	char send_buf[MSG_LEN];
	sprintf(send_buf, "CMD_M_OUT|%s|%s|", this->client_name.c_str(), this->inchat_room.c_str());
	flag = send(this->c2s_socket, send_buf, strlen(send_buf), 0);
	if(flag <= 0)
	{
		fprintf(stderr, "Line: %d. Send msg to server failed!\n", __LINE__);
	}
	printf("Already quit from chat room.\n");
	this->inchat_flag = 0;
	room_clear();
}


int messenger_client::init_room_info(const char *chatroom)
{
	#ifdef DEBUG
		printf("DEBUG: room info init function.\n");
	#endif

	int socket_len =0, flag =0;
	char send_buf[MAX_LEN], recv_buf[MAX_LEN];
	sprintf(send_buf, "CMD_REQUEST_ROOMINFO|%s|%s|", (this->client_name).c_str(), chatroom);
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
	
	printf("Waiting for loading the room info...\n");
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

		if(strncmp(recv_buf, "CMD_USER_ENTER", strlen("CMD_USER_ENTER")) == 0)
		{
			#ifdef DEBUG
				printf("DEBUG: receive CMD_USER_ENTER.\n");
            		#endif
			char buf_tmp[MSG_LEN], *m_cmd, *m_name, *m_addr;
			memset(buf_tmp, 0, MSG_LEN);
			memcpy(buf_tmp, recv_buf, strlen(recv_buf));
			m_cmd = strtok(buf_tmp, "|");
			m_name = strtok(NULL, "|");
			m_addr = strtok(NULL, "|");
			#ifdef DEBUG
				printf("DEBUG: request friend information: m_cmd=%s, m_name=%s, m_addr= %s.\n", m_cmd, m_name, m_addr);
			#endif
			add_room_list(string(m_name), string(m_addr));
			//add2_online_list(string(f_name), string(f_addr));
		}

		if(strncmp(recv_buf, "CMD_ROOMLIST", strlen("CMD_ROOMLIST")) == 0)
                {
                        #ifdef DEBUG
                                printf("DEBUG: receive CMD_ROOMLIST.\n");
                        #endif
                        char buf_tmp[MSG_LEN], *r_cmd, *r_name;
                        memset(buf_tmp, 0, MSG_LEN);
                        memcpy(buf_tmp, recv_buf, strlen(recv_buf));
                        r_cmd = strtok(buf_tmp, "|");
                        r_name = strtok(NULL, "|");
                        #ifdef DEBUG
                                printf("DEBUG: request roomlist information: r_cmd=%s, r_name=%s.\n", r_cmd, r_name);
                        #endif
                        add_roomlist(string(r_name));
		}
		
		if(strncmp(recv_buf, "CMD_M_OUT", strlen("CMD_M_OUT")) == 0)
		{
                        #ifdef DEBUG
                                printf("DEBUG: receive CMD_M_OUT.\n");
                        #endif
			char buf_tmp[MSG_LEN], *m_cmd, *m_name;
			memset(buf_tmp, 0, MSG_LEN);
			memcpy(buf_tmp, recv_buf, strlen(recv_buf));
			m_cmd = strtok(buf_tmp, "|");
			m_name = strtok(NULL, "|");
			#ifdef DEBUG
				printf("DEBUG: request friend information: m_cmd=%s, m_name=%s.\n", m_cmd, m_name);
			#endif

			while(m_name != NULL)
			{
				add2_friend_list(string(m_name));
				m_name = strtok(NULL, "|");
			}
			break;
		}
		#ifdef DEBUG
			printf("DEBUG: end of one round reset recv_buf.\n");
		#endif
		memset(recv_buf, 0, sizeof(recv_buf));
		#ifdef DEBUG
			printf("DEBUG: after reset recv_buf: %s. then new recv\n",recv_buf);
		#endif
		socket_len = 0;
		socket_len = recv(this->c2s_socket, recv_buf, MSG_LEN, 0);
		#ifdef DEBUG
			printf("DEBUG: after recv, recv_buf: [%s].\n",recv_buf);
		#endif
	}
	
	if(socket_len <= 0)
	{
		fprintf(stderr, "Line: %d. recv msg failed!\n", __LINE__);
		return -1;
	}
	else
	{
		#ifdef DEBUG
			printf("DEBGU: recv msg: %s.\n", recv_buf);
		#endif
	}
	disp_room_list();
	disp_member_list();
	return 0;
}

int messenger_client::init_room_member(string room_name)
{

}

int messenger_client::init_room_listen_socket()
{

}

void messenger_client::room_clear()
{
	fprintf(stderr, "Line: %d. quit room memery reset.\n", __LINE__);
	int i = 0;
	vector<messenger_user>().swap(this->chat_member);
	vector<string>().swap(this->file_list);
	vector<string>().swap(this->room_list);
	this->inchat_room.empty();
	this->thread_chat_finish_flag = 0;
	fprintf(stderr, "Line: %d. logout memreset finished.\n", __LINE__);
}

void messenger_client::s2c_room_msg_handler(const char *msg)
{
	#ifdef DEBUG
		printf("DEBUG: Received s2c_room_msg_handler: [%s]!\n", msg);
	#endif
	//fprintf(stderr, "Line: %d. Received server msg to room handler: [%s]!\n", __LINE__, msg);
	switch(parse_msg(msg))
	{
		case CMD_ENTER_SUCCESSED:
			s2c_enter_room(msg);
			break;
		case CMD_USER_ENTER:
			s2c_user_enter(msg);
			break;
		case CMD_M_OUT:
			s2c_m_out(msg);
			break;
		case CMD_MSG_ROOM:
			s2c_msg_room(msg);
			break;
		/*case CMD_DOWNLOAD_FINISHED:
			s2c_msg(msg);
			break;*/
		
		default:
			printf("Unknown server to client cmd [%s].\n", msg);
			break;
	}
}

void messenger_client::s2c_enter_room(const char *msg)
{
	fprintf(stderr, "Line: %d. Line: received msg: [%s]!\n", __LINE__, msg);
	char *user_cmd, *user_name, *user_addr,str[strlen(msg) + 1];
	memcpy(str, msg, strlen(msg));
	user_cmd = strtok(str, "|");
	user_name = strtok(NULL, "|");
	user_addr = strtok(NULL, "|");
	printf("User [%s] is enter chatroom [%s] now.\n", user_name, inchat_room.c_str());
	add_room_list(string(user_name), string(user_addr));
	//add2_online_list(user_name, user_addr);
	#ifdef  DEBUG
		printf("DEBUG: chat_member size %d.\n", chat_member.size());
	#endif
	disp_room_list();
	disp_member_list();
}

void messenger_client::s2c_user_enter(const char *msg)
{
	fprintf(stderr, "Line: %d. Line: recieved msg : [%s]!\n", __LINE__, msg);
	char *user_cmd, *user_name, *user_addr,str[strlen(msg) + 1];
        memcpy(str, msg, strlen(msg));
        user_cmd = strtok(str, "|");
        user_name = strtok(NULL, "|");
        user_addr = strtok(NULL, "|");
        printf("User [%s] is enter chatroom [%s] now.\n", user_name, inchat_room.c_str());
        add_room_list(string(user_name), string(user_addr));

	#ifdef  DEBUG
                printf("DEBUG: chat_member size %d.\n", chat_member.size());
        #endif
	printf("\n **********The current chatroom is [%s]*****************\n", this->inchat_room.c_str());
        disp_room_list();
        disp_member_list();

}

void messenger_client::s2c_m_out(const char *msg)
{
	fprintf(stderr, "Line: %d. Line: recieved msg: [%s]!\n", __LINE__, msg);
	char *user_cmd, *user_name, *room_name, *room_msg, str[strlen(msg) + 1];
	memcpy(str, msg, strlen(msg));
	user_cmd = strtok(str, "|");
	user_name = strtok(NULL, "|");
	room_name = strtok(NULL, "|");
	//room_msg = strtok(NULL, "|");
	if(strncmp(room_name, this->inchat_room.c_str(), strlen("room1")) == 0)
	{
		del_room_list(user_name);
		fprintf(stderr, "Line: %d. del_room_list of user [%s] successed!\n", __LINE__, user_name);
		disp_room_list();
		disp_member_list();
	}
	else
	{
		fprintf(stderr, "Line: %d. Chat room not match!\n", __LINE__);
	}
}

void messenger_client::s2c_msg_room(const char *msg)
{
	char *user_cmd, *user_name, *room_name, *room_msg, str[strlen(msg) + 1];
	memcpy(str, msg, strlen(msg));
	user_cmd = strtok(str, "|");
	user_name = strtok(NULL, "|");
	room_name = strtok(NULL, "|");
	room_msg = strtok(NULL, "|");
	if(strncmp(room_name, this->inchat_room.c_str(), strlen("room1")) == 0)
	{
		printf("\n[%s] send a msg >>>>  %s\n\n", user_name, room_msg);
	}
	else
	{
		fprintf(stderr, "Line: %d. Chat room not match!\n", __LINE__);
	}
}

void messenger_client::add_room_list(string f_name)
{
	int i = 0;
	messenger_user client;
	client.name = f_name;
	client.socket = -1;
	for( i = 0; i<this->chat_member.size();++i)
	{
		if((this->chat_member)[i].name == f_name)
			return;
	}
	this->chat_member.push_back(client);
}

void messenger_client::add_room_list(string f_name, string f_addr)
{
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
	for( i = 0; i<this->chat_member.size();++i)
	{
		if((this->chat_member)[i].name == f_name)
			return;
	}
	this->chat_member.push_back(client);
}

void messenger_client::add_roomlist(string r_name)
{
	int i = 0;
        for( i = 0; i<this->room_list.size();++i)
        {
                if((this->room_list)[i] == r_name)
                        return;
        }
        this->room_list.push_back(r_name);
}

void messenger_client::del_room_list(int socket)
{

}

void messenger_client::del_room_list(string user_name)
{
	int i = 0;
	vector<messenger_user>::iterator it=chat_member.begin();
        for( ; it != chat_member.end(); ++it)
        {
                if((*it).name == user_name)
		{
                        (this->chat_member).erase(it);
			fprintf(stderr, "Line: %d. del_room_list delete user [%s] successed!\n", __LINE__, user_name.c_str());
			return;
		}
        }
	fprintf(stderr, "Line: %d. There is no user_name [%s] during del_room_list!\n", __LINE__, user_name.c_str());

}

void messenger_client::disp_room_list()
{
	printf("\n **************Chatroom***************:\n ");
	if(this->room_list.size() == 0)
	{
		printf("You don't have chatroom list.\n");
		return;
	}

	int i = 0;
	for(i = 0; i < this->room_list.size(); ++i)
	{
		printf("[%s], ", (this->room_list)[i].c_str());
	}
	printf("\n");
}

void messenger_client::disp_member_list()
{
	printf("\n **************Chatroom members***************:\n ");
	if(this->chat_member.size() == 0)
	{
		printf("You don't have member in this chat room.\n");
		return;
	}

	int i = 0;
	for(i = 0; i < this->chat_member.size(); ++i)
	{
		printf("[%s], ", (this->chat_member)[i].name.c_str());
	}
	printf("\n");
}

void messenger_client::disp_file_list()
{
	printf("\n **************Chatroom file list***************:\n ");
        if(this->file_list.size() == 0)
        {
                printf("You don't have file in this chatroom.\n");
                return;
        }

        int i = 0;
        for(i = 0; i < this->file_list.size(); ++i)
        {
                printf("[%s], ", (this->file_list)[i].c_str());
        }
        printf("\n");

}

messenger_user *messenger_client::get_room_info(string r_name)
{
	return NULL;
}

int main(int argc, char const *argv[])
{
	if (argc != 2)
	{
		printf("Usage: messenger_client configuration_file\n");
		return 0;
	}
	else
	{
		#ifdef DEBUG
			printf("DEBUG:messenger_client configuration file received. \n");
		#endif
	}
	
	messenger_client *msg_client = new messenger_client(argv[1]);
	if(msg_client == NULL)
	{
		printf("New messenger client create failed.\n");
		return 0;
	}
	else
	{
		#ifdef DEBUG
			printf("DEBUG: msg client created successful.\n");
		#endif
	}

	ptr_this = msg_client;
	#ifdef DEBUG
		printf("ptr_this pointer successful.\n");
	#endif
	signal(SIGINT, sig_int_handler);
	int init;
	init = msg_client->init_socket();
	#ifdef DEBUG
		printf("DEBUG: init_socket successful.\n");
	#endif
	msg_client->print_start(init);
	#ifdef DEBUG
		printf("DEBUG: print_start successful.\n");
	#endif
	if(init)
	{
		#ifdef DEBUG
			printf("DEBUG: msg client socket init successful. \n");
		#endif
		return 0;
	}

	msg_client->loop();
	
	//LOG
	return 0;
}
