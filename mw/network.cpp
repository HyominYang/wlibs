#include <wesock.h>

#define BUFF_MAX 1024 * 1024 * 10 // 10 Mega-bytes

#include <iostream>
using namespace std;

void *network_table(void *obj)
{
	struct wsock_table *table = (struct wsock_table *) obj;

	cout<<"execute network-table."<<endl;
	wsock_table_run(table);
	cout<<"network-table is terminated."<<endl;

	return NULL;
}

#if 0
void f_disconn (struct wsock_table *table, struct wsock *clnt)
{
	printf("%s:%d is disconnected.\n", clnt->addr_info.ch_ip, clnt->addr_info.h_port);
	quit = 1;
}

void f_recv (struct wsock_table *table, struct wsock *clnt, unsigned char *buff, int read_len, int *offset)
{
	timeout = 0;
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

	while(1)
	{
		sleep(1);
		if(table->element_count_current == 0) {
			// connect to server
			//cout<<"try connect to server."<<endl;
#if 0
			if((wsock = wsock_add_new_tcp_client(
					table,
					argv[1],
					atoi(argv[2]),
					0,
					5,
					NULL,
					&f_recv,
					&f_disconn)) == NULL) {
				printf("wsock_add_new_tcp_client() is failed.\n");
				return -1;
			}
#endif
		}
	}

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
