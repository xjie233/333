#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/msg.h>

#define   MSG_FILE "/tmp/msg_server"    
#define   BUFFER 255    
#define   PERM S_IRUSR | S_IWUSR    

#define 	OK			1 
#define 	ERR			0

// msg type
#define   TYPE_SERVER		1000
#define   TYPE_SERVER_STR	"1000"

// msg target string
#define   SERVER_STR		"SERVER"
#define   TO_ALL_STR		"ALL"

// send cmd
// L, I, O is send to server
#define 	CMD_LIST		'L'
#define 	CMD_LOGIN		'I'
#define 	CMD_LOGOUT		'O'
#define 	CMD_TOALL 		'A'

// C, F send to others
#define 	CMD_CHAT		'C'
#define 	CMD_SEND_FILE	'F'

// CMD:FROM:TIME:DATA 
#define		DATA_LEN	4
#define		OFT_CMD		0
#define		OFT_FRM		1
#define		OFT_TIM		2
#define		OFT_DAT		3
#define		DATA_TOK	":"

// USR_ID:USR_NAME:TIME
#define		USER_DATA_LEN	3
#define		OFT_USR_ID		0
#define		OFT_USR_NM		1
#define		OFT_LOGIN_TM	2
#define		USER_DATA_TOK	"#"

// id admin
#define 	START_ID	1

//-------------------------------------+
// List operations
struct list_head {
	struct list_head *next;
};

// init a new list named name
#define LIST_INIT(name)	\
	struct list_head name = {NULL}

static inline int list_add(struct list_head * head, struct list_head* new){
	new->next = head->next;
	head->next = new;
}

static inline int list_delete(struct list_head * head, struct list_head* target){
	while(head){
		if(head->next == target){
			head->next = target->next;
			return 0;
		}
		head = head->next;
	}
	return -1;
}
//-------------------------------------+

// online status
enum status{ online, offline, invisible };

// available id 
int available_id = START_ID;

// user struct to save user informations
struct user{
	struct list_head list;
	int id;
	char name[32];
	enum status status;;
	long login_time;
};

// message struct
struct message
{    
	long mtype;
	char buffer[BUFFER+1];    
}msg_snd, msg_rcv;    

int msgid = 0;

// send a format message to client
// CMD:FROM:TIME:DATA 
int send_msg(long type, int cmd, int from_id, char * data){
	sprintf(msg_snd.buffer, "%c:%d:%d:%s", cmd, from_id, time(NULL), data);
	msg_snd.mtype = type;
	if(msgsnd(msgid, &msg_snd, strlen(msg_snd.buffer)+1, 0) < 0)
		return ERR;
	else
		return OK;
}

inline char *time2str(long time, char* buf){
	struct tm *t = localtime(&time);
	strftime(buf, 32, "%Y-%m-%d-%H:%M:%S", t);
	return buf;
}

int init_msg_queue(){
	key_t key;
	if((key = ftok(MSG_FILE, 'a')) == -1){
		perror("ftok");
		exit(1);
	} 

	printf("Key:%d\n", key);
	if((msgid = msgget(key, PERM | IPC_CREAT)) == -1)
	{   
		perror("msgget");
		exit(1);
	}       
	printf("msgid = %d\n", msgid);
	return msgid;
}       

int process_msg(char* buffer);  
