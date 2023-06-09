/* msg_svr.c */  

#include "msg.h"   

LIST_INIT(msg_list_head); 

int main()    
{    
	msgid = init_msg_queue();

	while(1)   
	{
		if(msgrcv(msgid, &msg_rcv, sizeof(struct message), TYPE_SERVER, 0) == -1)
			perror("msgrcv");
		else
			printf("Get: %s\n", msg_rcv.buffer);

		// process message
		process_msg(msg_rcv.buffer);
	}    
	exit(0);    
}

// process message received from message queue
// message format:
// CMD:TARGET:FROM:TIME:DATA
int process_msg(char* buffer){
	char * data[DATA_LEN];
	char * str, *subtoken;
	int i;
	memset(data, NULL, sizeof(data));
	for(str = buffer, i = 0; ; str = NULL, i++){
		subtoken = strtok(str, DATA_TOK);
		if(subtoken == NULL)
			break;
		data[i] = subtoken;
#ifdef _DEBUG
		printf("> data[%d] = %s\n", i, subtoken);
#endif
	}
	// data format error
	if(i != DATA_LEN)
		return ERR;

	char info[256];
	char buf[256];
	struct user* u ;
	struct list_head* p;

	// send to server cmd
	switch(data[OFT_CMD][0]){
	case CMD_LIST:
		bzero(buf, sizeof(buf));
		p = (&msg_list_head)->next;
		while(p){
			u= (struct user*)p;
			sprintf(info, "%d#%s#%ld#", u->id, u->name, u->login_time);
#ifdef _DEUBG
			printf("u->name = %s\n", u->name);
#endif
			strcat(buf, info);
			p = p->next;
		}
		if(p != msg_list_head.next){
			// delete the end '#'
			buf[strlen(buf) - 1] = 0;
		}
		send_msg(atol(data[OFT_FRM]), CMD_LIST, TYPE_SERVER, buf);
		break;

	case CMD_LOGIN:
		u = (struct user *)malloc(sizeof(struct user));
		if(NULL == u){
			perror("malloc");
			return ERR;
		}
		// add to list
		list_add(&msg_list_head, &(u->list));
		u->id = available_id++;
		strcpy(u->name, data[OFT_FRM]);
		u->status = online;
		u->login_time = atol(data[OFT_TIM]);

		// login ok echo msg
		msg_snd.mtype = atol(data[OFT_DAT]);                
		sprintf(msg_snd.buffer, "%d", u->id);
		msgsnd(msgid, &msg_snd, strlen(buf)+1, 0);  
		break;

	case CMD_LOGOUT:
		p = (&msg_list_head)->next;
		while(p){
			u= (struct user*)p;
			// find the user who request logout
			if(u->id == atoi(data[OFT_FRM])){
				if(send_msg(u->id, CMD_LOGOUT, TYPE_SERVER, "Logout OK!") == OK){
					list_delete(&msg_list_head, p);
					free(p);
				}else{
					return ERR;
				}
				break;
			}
			p = p->next;
		}
		break;

	case CMD_TOALL:
		// send to all online client
		p = (&msg_list_head)->next;
		while(p){
			u= (struct user*)p;
			send_msg(u->id, CMD_CHAT, data[OFT_FRM], data[OFT_DAT]);
			p = p->next;
		}
		break;
	default:
		return ERR;
	}
	return OK;
}

