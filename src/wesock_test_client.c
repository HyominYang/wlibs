#include "wesock.h"

void f_recv (struct wsock_table *table, struct wsock *clnt, char *buff, int read_len, int *offset)
{
	printf("%s:%d receive data(%d bytes).", clnt->addr_info.ch_ip, clnt->addr_info.h_port, read_len);
}

void f_disconn (struct wsock_table *table, struct wsock *clnt)
{
	printf("%s:%d is disconnected.", clnt->addr_info.ch_ip, clnt->addr_info.h_port);
}

void *f_runner(void *obj)
{
	struct wsock_table *table = (struct wsock_table *) obj;

	printf("Excute Table!!!\n");
	wsock_table_run(table);
	printf("Stop Table!!!\n");

	return NULL;
}

int main()
{
	struct wsock_table table;
	struct wsock *wsock;
	pthread_t pid;
	char *buff;
	int buff_len, i;

	buff = malloc(1024 * 1024 * 10);
	if(buff == NULL) {
		printf("malloc() is failed.\n");
		return -1;
	}

	if(wsock_create_tcp_table(&table, 1, 1024 * 1024, 60) < 0) {
		printf("wsock_create_tcp_table() is failed.\n");
		return -1;
	}

	if((wsock = wsock_add_new_tcp_client(&table, "15.1.2.91", 0x916, 0, 5, NULL,
			f_recv,
			f_disconn)) == NULL) {
		printf("wsock_add_new_tcp_client() is failed.\n");
		return -1;
	}
	pthread_create(&pid, NULL, f_runner, (void *) &table);

	for(i=0; i<10; i++) {
		buff_len = (rand() % (1024 * 1024 * 10)) + 1;
		printf("buff_len : %d\n", buff_len);
		if(wsock_send(wsock, buff, buff_len) < 0) {
			printf("wsock_send() is failed.\n");
			wsock->table->flag_exit = 1;
			break;
		}
	}

	return 0;
}
