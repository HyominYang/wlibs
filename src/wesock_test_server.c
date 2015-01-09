#include "wesock.h"

void f_conn (struct wsock_table *table, struct wsock *serv, struct wsock *clnt)
{
	printf("%s:%d is connected.\n", clnt->addr_info.ch_ip, clnt->addr_info.h_port);
}

void f_recv (struct wsock_table *table, struct wsock *clnt, unsigned char *buff, int read_len, int *offset)
{
	static int rd_len = 0;
	rd_len += read_len;
	printf("%s:%d receive data(%d bytes. total %d).\n", clnt->addr_info.ch_ip, clnt->addr_info.h_port, read_len, rd_len);
}

void f_disconn (struct wsock_table *table, struct wsock *clnt)
{
	printf("%s:%d is disconnected.\n", clnt->addr_info.ch_ip, clnt->addr_info.h_port);
}

int main(int argc, char **argv)
{
	struct wsock_table table;	

	if(wsock_create_tcp_table(&table, 5, 1024 * 1024, 60) < 0) {
		printf("error : wsock_create_tcp_table()\n");
		return -1;
	}
	if(wsock_add_new_tcp_server(&table, "0.0.0.0", 10348, 5, 4, NULL, 
			f_conn,
			f_recv,
			f_disconn
			) < 0) {
		printf("Error : wsock_add_new_tcp_server() is failed.\n");
	}
	printf("Server Start!!!\n");
	wsock_table_run(&table);
	printf("Server Stop!!!\n");

	return 0;
}
