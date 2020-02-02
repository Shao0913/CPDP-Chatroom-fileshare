#ifndef __CPDP_CONFIG_H__
#define __CPDP_CONFIG_H__

//#include <stdio.h>
//#include <iostream>
#include <string>

//using namespace std;
using std::string;

//#define DEBUG

#define MAX_U2S 30 	//max user to sever is 30
#define MSG_LEN 4095	//max messeng length
#define MAX_THREAD_NUM 50 //max thread run by server
#define MAX_CMD_LEN 256	//max cmd length
#define MAX_BUF_LEN 4095	//max messenge buff length
#define MAX_C2C 20	//max number of client connection
#define MAX_LEN	4095	//max lenth of sending buffer
#define FILE_NAME_LEN 100	//max lenth of file name
#define MAXPID 4	//max pid table size

typedef enum CMD_TYPE
{
	CMD_REGISTER = 1,
	CMD_LOGIN = 2,
	CMD_LOGOUT = 3,
	CMD_INVITE = 4,
	CMD_ACCEPT = 5,
	CMD_REQUEST_FRIEND_ID = 6,
	CMD_EXIT = 7,
	CMD_UNKNOWN = 8,

	CMD_SENDMSG = 9,
	CMD_FD_ONLINE = 10,
	CMD_FD_OFFLINE = 11,
	CMD_S2CMSG = 12,
	CMD_C2CMSG = 13,
	CMD_NOWONLINE = 14,
	CMD_REGISTER_SUCCESSED = 101,
	CMD_LOGIN_SUCCESSED = 102,
	
	//term project cmd type
	CMD_ENTER_ROOM = 51,	//enter chat room
	CMD_MSG_ROOM = 52,	//send message to current chat room
	CMD_UPLOAD = 53,	//upload file to current chat room
	CMD_DOWNLOAD = 54,	//down load file
	QUIT_ROOM = 55,		//quit current chat room
	CMD_INROOM = 56,	//server send msg inroom to client when client is already inroom
	CMD_ENTER_SUCCESSED = 57,	//enter chatroom successed
	CMD_USER_ENTER = 58,		//user enter chatroom notice other user and update
	CMD_REQUEST_ROOMINFO = 59,	//user request for room info
	CMD_M_OUT = 60,		//user exit chat room  
	CMD_EDIT =61,
	CMD_EDIT_DONE = 62,
}cmd_type;

typedef struct messenger_user{
	int socket;
	string name;
	string pwd;
	string addr;
}messenger_user;

#endif
