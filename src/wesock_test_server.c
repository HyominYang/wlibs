#include "wesock.h"

void f_conn (struct wsock_table *table, struct wsock *serv, struct wsock *clnt)
{
	printf("%s:%d is connected.", clnt->addr_info.ch_ip, clnt->addr_info.h_port);
}

void f_recv (struct wsock_table *table, struct wsock *clnt, char *buff, int read_len, int *offset)
{
	printf("%s:%d receive data(%d bytes).", clnt->addr_info.ch_ip, clnt->addr_info.h_port, read_len);
}

void f_disconn (struct wsock_table *table, struct wsock *clnt)
{
	printf("%s:%d is disconnected.", clnt->addr_info.ch_ip, clnt->addr_info.h_port);
}

int main(int argc, char **argv)
{
	struct wsock_table table;	

	if(wsock_create_tcp_table(&table, 5, 1024 * 1024, 60) < 0) {
		printf("error : wsock_create_tcp_table()\n");
		return -1;
	}
	wsock_add_new_tcp_server(&table, "127.0.0.1", 0x916, 1, 4, NULL, 
			f_conn,
			f_recv,
			f_disconn
			);
	printf("Server Start!!!\n");
	wsock_table_run(&table);
	printf("Server Stop!!!\n");

	return 0;
}
