#include "global.h"

char *buff = NULL;
int timeout = 0;
int quit = 0;
int state=0;

int flag_connected;
void f_conn (struct wsock_table *table, struct wsock *serv, struct wsock *clnt);
void f_recv (struct wsock_table *table, struct wsock *clnt, unsigned char *buff, int read_len, int *offset);
void f_disconn (struct wsock_table *table, struct wsock *clnt);
void message_parser(struct wsock *clnt, char *buff);
void *f_runner(void *obj)
{
	struct wsock_table *table = (struct wsock_table *) obj;

	printf("Excute Table!!!\n");
	wsock_table_run(table);
	printf("Stop Table!!!\n");

	return NULL;
}
void procedure(struct wsock *wsock);
int main(int argc, char **argv)
{
	struct wsock_table table;
	struct wsock *wsock;
	pthread_t pid;
	int buff_len, i;

	if(argc < 3) {
		printf("USAGE: bin <ip address> <port>\n\n\n");
		return -1;
	}

	buff = malloc(1024 * 1024 * 10);
	if(buff == NULL) {
		printf("malloc() is failed.\n");
		return -1;
	}

	if(wsock_create_tcp_table(&table, 1, 1024 * 1024, 60) < 0) {
		printf("wsock_create_tcp_table() is failed.\n");
		return -1;
	}

	if((wsock = wsock_add_new_tcp_client(&table, argv[1], atoi(argv[2]), 0, 5, f_conn,
			f_recv,
			f_disconn)) == NULL) {
		printf("wsock_add_new_tcp_client() is failed.\n");
		return -1;
	}
	pthread_create(&pid, NULL, f_runner, (void *) &table);
loop:
	sleep(1); timeout++;

	procedure(wsock);

	if(quit == 0 && timeout < 100) {
		goto loop;
	}

	sleep(10);

	return 0;
}


void f_disconn (struct wsock_table *table, struct wsock *clnt)
{
	printf("%s:%d is disconnected.\n", clnt->addr_info.ch_ip, clnt->addr_info.h_port);
	quit = 1;
}

void f_conn (struct wsock_table *table, struct wsock *serv, struct wsock *clnt)
{
	timeout = 0;
	printf("%s:%d is connected.\n", clnt->addr_info.ch_ip, clnt->addr_info.h_port);
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
	message_parser(clnt, buff);
	// end TODO

	if(msg->len < tot_len) {
		memcpy(buff, buff+msg->len, tot_len - msg->len);
		*offset = 0;
		read_len = tot_len - msg->len;
		goto f_recv_loop;
	}
}

void procedure(struct wsock *wsock)
{
	msg_header_t *msg = (msg_header_t *) buff;
	char *data = ((char *)buff + sizeof(msg_header_t));
	if(state == 0) {
		msg->len = sizeof(msg_header_t);
		msg->command = REQ_DOWNLOAD_PACK;
		if(wsock_send(wsock, msg, msg->len) < 0) {
			printf("send failed.\n");
		} else {
			printf(">> REQ_DOWNLOAD_PACK\n");
			state = 1;
		}
	}

	else if(state == 1) {
	}
}

void message_parser(struct wsock *clnt, char *buff)
{
	msg_header_t msg = *((msg_header_t *)buff);
	msg_header_t *res_msg = (msg_header_t *)buff;
	int data_len = msg.len - sizeof(msg_header_t);

	char *data = (char *) (buff + sizeof(msg_header_t));

	if(msg.command == REQ_GET_LICENCE) {
	}
	else if(msg.command == REQ_GET_VERSION) {
	}
	else if(msg.command == REQ_DOWNLOAD_PACK) {
		//int fd = open("./a.txt", O_TRUNC|O_CREAT|O_SYNC);
		int fd = open("./a.txt", O_RDONLY);
		if(fd < 0) {
			printf("open error\n");
			return;
		}
		int read_len = read(fd, buff+sizeof(msg_header_t), BUFF_MAX);
		close(fd);
		if(read_len <= 0) {
			res_msg->len = read_len + sizeof(msg_header_t);
			res_msg->command = RES_FAILED;
		} else {
			res_msg->len = read_len + sizeof(msg_header_t);
			res_msg->command = RES_DOWNLOAD_PACK;
		}
		if(wsock_send(clnt, buff, read_len) < 0) {
			printf("send failed.\n");
		}
	}
	if(msg.command == RES_DOWNLOAD_PACK) {
		printf(">> RES_DOWNLOAD_PACK\n");
		int fd = open("./b.txt", O_WRONLY|O_TRUNC|O_SYNC|O_CREAT);
		if(fd < 0) {
			printf("open error\n");
			return;
		}
		write(fd, data, data_len);
		close(fd);
	}
}








