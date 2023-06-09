/* msg_client.c */  
#include "msg.h"   

int userid = 0;

char name[32] = "";

void print_menu(void){
	printf("\t+----------------------------------+\n");
	printf("\t+    Chat Room V1.0 2013.01.08     +\n");
	printf("\t+----------------------------------+\n");
	printf("\t+ User Commands as follows:        +\n");
	printf("\t+                                  +\n");
	printf("\t+ l: list all online user          +\n");
	printf("\t+ i: Login                         +\n");
	printf("\t+ o: logOut                        +\n");
	printf("\t+ c: Chat with other online user   +\n");
	printf("\t+ a: Chat with all online user     +\n");
	printf("\t+ f: transfer a File to others     +\n");
	printf("\t+ h: Help                          +\n");
	printf("\t+----------------------------------+\n");
}

int get_choice(){
	printf("%s# ", name);
	int answer = getchar();	// eat <Enter>
	while(getchar() != '\n');	// eat <Enter>
	//putchar(answer);
	return answer;
}

void func(int sig){
	printf("\n%s# ", name);
	fflush(stdout);
}

int send_to(int target, int cmd, char *data){
	return send_msg(target, cmd, userid, data);
}

int send_server(int cmd, char *data){
	return send_msg(TYPE_SERVER, cmd, userid, data);
}

int chat(){
	if(strlen(name) == 0){
		printf("You are not login!\n");
		return ERR;
	}

	char id[32];
	char data[256];
	char buf[256];
	printf("To: [USR_ID] ");
	fflush(stdout);
	if(fgets(id, sizeof(id), stdin) == NULL){
		perror("fgets");
		return ERR;
	}
	sprintf(data, " %s > ", name);
	id[strlen(id) - 1] = 0;

	printf(">> ");
	fflush(stdout);
	if(fgets(buf, sizeof(buf), stdin) == NULL){
		perror("fgets");
		return ERR;
	}
	strcat(data, buf);
	data[strlen(data) - 1] = 0;
	send_to(atoi(id), CMD_CHAT, data);
}

int chat_all(){
	if(strlen(name) == 0){
		printf("You are not login!\n");
		return ERR;
	}

	char data[256];
	char buf[256];
	sprintf(data, " %s To all > ", name);

	printf("To all >> ");
	fflush(stdout);
	if(fgets(buf, sizeof(buf), stdin) == NULL){
		perror("fgets");
		return ERR;
	}
	strcat(data, buf);
	data[strlen(data) - 1] = 0;
	send_to(TYPE_SERVER, CMD_TOALL, data);
}

int login(){
	printf("username: \n");
	if(fgets(name, sizeof(name), stdin) == NULL){
		perror("fgets");
		return ERR;
	}
	name[strlen(name) - 1] = 0;
	int rand_type = random();
	time_t t;
	time(&t);
	sprintf(msg_snd.buffer, "%c:%s:%ld:%d", CMD_LOGIN, name, t, rand_type);
#ifdef _DEBUG
	printf("%s\n", msg_snd.buffer);
#endif
	// get a random type to login server
	msg_snd.mtype = TYPE_SERVER;
	if(msgsnd(msgid, &msg_snd, strlen(msg_snd.buffer)+1, 0) < 0){
		perror("msgsnd");
		return ERR;
	}

	// wait server response
	if(msgrcv(msgid, &msg_rcv, sizeof(msg_rcv), rand_type, 0) < 0){
		return ERR;
	} else{
		userid = atol(msg_rcv.buffer);
		printf("Login OK id = %d\n", userid);
		return OK;
	}
}

int logout(){
	if(strlen(name) == 0){
		return ERR;
	}
	send_server(CMD_LOGOUT, "Logout");
	// wait server response
	if(msgrcv(msgid, &msg_rcv, sizeof(msg_rcv), userid, 0) < 0){
		return ERR;
	}else{
		userid = 0;
		printf("Logout OK\n");
		return OK;
	}
}

void format_user_list(char * buffer){
	char * data[USER_DATA_LEN];
	char * str, *subtoken;
	char time_buf[24];
	int i, n;
	memset(data, NULL, sizeof(data));
	printf("----------------------------------\n");	
	printf(" _ID_  USR_NAME  Login_Time \n");	
	printf("----------------------------------\n");	
	for(str = buffer, i = 0, n = 1; ; str = NULL, i++){
		subtoken = strtok(str, USER_DATA_TOK);
		if(subtoken == NULL)
			break;
		data[i] = subtoken;
#ifdef _DEBUG
		printf("> data[%d] = %s\n", i, subtoken);
#endif
		if(i != 0 && i % 2 == 0){
			printf(" %4s  %8s  %s\n", data[OFT_USR_ID], data[OFT_USR_NM], time2str(atol(data[OFT_LOGIN_TM]), time_buf));	
			n++;
			i = -1;
		}
	}
}

void * receiver_looper(void * p){
	if(userid == 0)
		return NULL;
	char * data[DATA_LEN];
	char * str, *subtoken;
	int i;
	while(1){
		if(msgrcv(msgid, &msg_rcv, sizeof(msg_rcv), userid, 0) < 0){
			perror("msgrcv");
			continue;
		}else{

#ifdef _DEBUG
			printf("%s received: %s\n", __func__, msg_rcv.buffer);
#endif
			memset(data, NULL, sizeof(data));
			for(str = msg_rcv.buffer, i = 0; ; str = NULL, i++){
				subtoken = strtok(str, DATA_TOK);
				if(subtoken == NULL)
					break;
				data[i] = subtoken;
#ifdef _DEBUG
				printf("> data[%d] = %s\n", i, subtoken);
#endif
			}
			// process received data
			// data format error
			if(i != DATA_LEN)
				continue;

			switch(data[OFT_CMD][0]){
			case CMD_LIST:
				if(strcmp(data[OFT_FRM], TYPE_SERVER_STR)){
					continue;
				}
				format_user_list(data[OFT_DAT]);
				break;
			case CMD_LOGOUT:
				if(strcmp(data[OFT_FRM], TYPE_SERVER_STR)){
					continue;
				}
				printf("> %s ", data[OFT_DAT]);
				printf("%s\n", time2str(atol(data[OFT_TIM]), data[OFT_DAT]));
				exit(0);
			case CMD_CHAT:		// print chat content
				printf("\n%s \n\t\t", data[OFT_DAT]);
				printf("%s\n", time2str(atol(data[OFT_TIM]), data[OFT_DAT]));
				printf("\n%s# ", name);
				fflush(stdout);
				break;
			case CMD_SEND_FILE:
				break;
			}
		}
	}
}


int main(int argc, char **argv)    
{    
	signal(SIGINT, func);
	//	signal(SIGQUIT, func);

	msgid = init_msg_queue();

	pthread_t thread;
	// print help menu
	print_menu();
	while(1){
		switch(get_choice()){
		case 'i':
			login();
			while(pthread_create(&thread, NULL, receiver_looper, NULL) < 0);
			break;
		case 'l':
			send_server(CMD_LIST, "List");
			sleep(1);
			break;
		case 'o':
			if(logout() == OK){
				pthread_join(thread, NULL);
				return 0;
			}else{
				printf("Not login quit\n");
				return 0;
			}
			break;
		case 'c':
			chat();
			break;
		case 'a':
			chat_all();
			break;
			break;
		case 'h':
			print_menu();
			break;
		case 'f':
			break;
		}
	}
}     

