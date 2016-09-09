#include <wesock.h>
#include <iostream>

#include "defines.h"
#include "msgtype.h"

#include <confr.h>

static int flag_run_table = 0;
using namespace std;

void message_parser(struct wsock *clnt, char *buff)
{
	msg_header_t *msg_h = (msg_header_t *) buff;
	msg_t *msg = (msg_t *) buff;

	char *data = (char *) &msg_h[1];

	//cout<<data<<endl;
	if(msg->command == REQ_GET_VERSION) {
		WConf ver;

		msg_t res_msg;
		memset(&res_msg, 0, sizeof(res_msg));
		res_msg.command = RES_GET_VERSION;

		string ver_filepath = "./s_ver";
		if(ver.read(ver_filepath) == false) {
			res_msg.d.number = 0;
		} else {
			res_msg.d.number = atoi(ver["version"].data());
		}
	}
}

void *network_table(void *obj)
{
	struct wsock_table *table = (struct wsock_table *) obj;

	cout<<"execute network-table."<<endl;
	flag_run_table = 1;
	wsock_table_run(table);
	cout<<"network-table is terminated."<<endl;

	exit(1);
	return NULL;
}

#if 1
void f_conn (struct wsock_table *table, struct wsock *serv, struct wsock *clnt)
{
	printf("%s:%d is connected.\n", clnt->addr_info.ch_ip, clnt->addr_info.h_port);
}

void f_disconn (struct wsock_table *table, struct wsock *clnt)
{
	printf("%s:%d is disconnected.\n", clnt->addr_info.ch_ip, clnt->addr_info.h_port);
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

	msg_header_t *msg = (msg_header_t *) buff;
	if(msg->len > tot_len) {
		*offset = tot_len;
		return;
	}

	// TODO: A message processing
	message_parser(clnt, (char *) buff);
	// end TODO

	if(msg->len < tot_len) {
		memcpy(buff, buff+msg->len, tot_len - msg->len);
		*offset = 0;
		read_len = tot_len - msg->len;
		goto f_recv_loop;
	}
}
#endif

void *network_procedure(void *obj)
{
	struct wsock_table *table = (struct wsock_table *) obj;
	struct wsock *wsock;

	if(wsock_add_new_tcp_server(table, (char *)"0.0.0.0", 48583, 5, 10, NULL,
			f_conn,
			f_recv,
			f_disconn) < 0) {
		printf("Error : wsock_add_new_tcp_server() is failed.\n");
		exit(1);
		return NULL;
	}

	while(1)
	{
		sleep(1);
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

	if(wsock_create_tcp_table(table, 11/*client cnt+1*/, BUFF_MAX, 60) < 0) {
		cout<<"wsock_create_tcp_table() is failed."<<endl;
		return -1;
	}

	if(pthread_create(&pid, NULL, network_table, (void *) table) != 0) {
		cout<<"Create a thread is failed. (network_table)"<<endl;
		return -1;
	}

	while(flag_run_table == 0) {
		usleep(1000);
	}
	sleep(1);

	if(pthread_create(&pid, NULL, network_procedure, (void *) table) != 0) {
		cout<<"Create a thread is failed. (network_procedure)"<<endl;
		return -1;
	}


	return 0;
}
