#include <wesock.h>
#include <iostream>

#include <confr.h>

#include "defines.h"
#include "msgtype.h"
#include "state.h"

extern int g_connected;
extern int g_state;
extern WConf g_conf;

extern void pstate(int state);

using namespace std;

void message_parser(struct wsock *clnt, char *buff)
{
	msg_t *msg = (msg_t *) buff;

	switch(msg->command) {
		case RES_DOWNLOAD:
		if(msg->data_len == 0) {
			pstate(SM_CHECK_BIN);
		} else {
			string filepath = g_conf["download_filepath"] + "/" + g_conf["download_filename"];
			int fd = open(filepath.data(), O_WRONLY|O_TRUNC|O_CREAT);
			if(fd < 0) {
				cout<<"Open error!!!"<<endl;
				pstate(SM_CHECK_BIN);
			} else {
				write(fd, msg->d.data, msg->data_len);
				sync();
				close(fd);
				pstate(SM_UPGRADE);
			}
		}
		break;
	}
}

void *network_table(void *obj)
{
	struct wsock_table *table = (struct wsock_table *) obj;

	cout<<"execute network-table."<<endl;
	wsock_table_run(table);
	cout<<"network-table is terminated."<<endl;

	exit(1);
	return NULL;
}

#if 1
void f_disconn (struct wsock_table *table, struct wsock *clnt)
{
	g_connected = 0;
	printf("%s:%d is disconnected.\n", clnt->addr_info.ch_ip, clnt->addr_info.h_port);
	g_state = SM_CHECK_BIN;
}

void f_recv (struct wsock_table *table, struct wsock *clnt, unsigned char *buff, int read_len, int *offset)
{
	printf("%s:%d receive data(%d bytes).\n", clnt->addr_info.ch_ip, clnt->addr_info.h_port, read_len);
	int tot_len;
f_recv_loop:
	tot_len = *offset + read_len;
	if(tot_len < sizeof(msg_header_t)) {
		*offset = tot_len;
		return;
	}

	msg_header_t *msg_h = (msg_header_t *) buff;
	if(msg_h->len > tot_len) {
		*offset = tot_len;
		return;
	}

	// TODO: A message processing
	message_parser(clnt, (char *) buff);
	// end TODO

	if(msg_h->len < tot_len) {
		memcpy(buff, buff+msg_h->len, tot_len - msg_h->len);
		*offset = 0;
		read_len = tot_len - msg_h->len;
		goto f_recv_loop;
	}
}
#endif

#define INIT_MSG() memset(buff, 0, sizeof(msg_t))
void *network_procedure(void *obj)
{
	struct wsock_table *table = (struct wsock_table *) obj;
	struct wsock *wsock;

	char *buff = (char *) new msg_t;
	if(buff == NULL) {
		cout<<"malloc is failed."<<endl;
		exit(1);
		return NULL;
	}

	msg_header_t *msg_h = (msg_header_t *) buff;
	char *data = (char *) &msg_h[1];
	msg_t *msg = (msg_t *) buff;
	while(1)
	{
		sleep(1);
		if(table->element_count_current == 0) {
			// connect to server
			//cout<<"try connect to server."<<endl;
#if 1
			if((wsock = wsock_add_new_tcp_client(
					table,
					(char *) "127.0.0.1",
					48583,
					0,
					5,
					NULL,
					&f_recv,
					&f_disconn)) == NULL) {
				printf("wsock_add_new_tcp_client() is failed.\n");
				//exit(1);
				//return NULL;
				g_connected = 0;
			} else {
				g_connected = 1;
			}
#endif
		} else if(g_connected == 1) {
#define msg_init() memset(msg, 0, sizeof(msg_t))
#define msg_calc() msg->len = 12 + msg->data_len
#define msg_send() wsock_send(wsock, msg, msg->len)
			if(g_state == SM_WAIT) {
				msg_init();
				msg->d.number = atoi(g_conf["version"].data());
				msg->data_len = 4;
				msg->command = REQ_DOWNLOAD;
				msg_calc();
				msg_send();
				pstate(SM_DOWNLOAD);
			}
		}
#if 0 // TEST_CODE
		msg_h->len = sizeof(msg_header_t);
		strcpy(data, "Hello, world!");
		msg_h->len += strlen(data) + 1;
		wsock_send(wsock, msg_h, msg_h->len);
#endif
	}

	exit(1);
	return NULL;
}

int create_network_procedure()
{
	pthread_t pid;
	struct wsock_table *table = NULL;

	if((table = new struct wsock_table) == NULL) {
		cout<<"malloc() failed. (wsock_table)"<<endl;
		return -1;
	}

	if(wsock_create_tcp_table(table, 1, BUFF_MAX, 60) < 0) {
		cout<<"wsock_create_tcp_table() is failed."<<endl;
		return -1;
	}

	if(pthread_create(&pid, NULL, network_table, (void *) table) != 0) {
		cout<<"Create a thread is failed. (network_table)"<<endl;
		return -1;
	}
	if(pthread_create(&pid, NULL, network_procedure, (void *) table) != 0) {
		cout<<"Create a thread is failed. (network_procedure)"<<endl;
		return -1;
	}


	return 0;
}

