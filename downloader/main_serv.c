#include "global.h"

void message_parser(struct wsock *clnt, char *buff);
void f_conn (struct wsock_table *table, struct wsock *serv, struct wsock *clnt)
{
	printf("%s:%d is connected.\n", clnt->addr_info.ch_ip, clnt->addr_info.h_port);
}
void f_recv (struct wsock_table *table, struct wsock *clnt, unsigned char *buff, int read_len, int *offset);
void f_disconn (struct wsock_table *table, struct wsock *clnt);
void *f_runner(void *obj)
{
	struct wsock_table *table = (struct wsock_table *) obj;

	printf("Excute Table!!!\n");
	wsock_table_run(table);
	printf("Stop Table!!!\n");

	return NULL;
}

int main(int argc, char **argv)
{
	struct wsock_table table;
	struct wsock *wsock;
	pthread_t pid;
	int buff_len, i;

	if(argc < 2) {
		printf("USAGE: bin <serv port>\n\n\n");
		return -1;
	}

	if(wsock_create_tcp_table(&table, 6, BUFF_MAX, 60) < 0) {
		printf("wsock_create_tcp_table() is failed.\n");
		return -1;
	}

	if(wsock_add_new_tcp_server(&table, "0.0.0.0", atoi(argv[1]), 5, 4, NULL,
			f_conn,
			f_recv,
			f_disconn) < 0) {
		printf("Error : wsock_add_new_tcp_server() is failed.\n");
		return -1;
	}
	pthread_create(&pid, NULL, f_runner, (void *) &table);

	printf("Server Start!!!\n");
	wsock_table_run(&table);
	printf("Server Stop!!!\n");

	return -1;
loop:
	sleep(1);
	goto loop;

	return 0;
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
	message_parser(clnt, buff);
	// end TODO

	if(msg->len < tot_len) {
		memcpy(buff, buff+msg->len, tot_len - msg->len);
		*offset = 0;
		read_len = tot_len - msg->len;
		goto f_recv_loop;
	}
}

void f_disconn (struct wsock_table *table, struct wsock *clnt)
{
	printf("%s:%d is disconnected.\n", clnt->addr_info.ch_ip, clnt->addr_info.h_port);
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
		printf("<< REQ_DOWNLOAD_PACK\n");
		//int fd = open("./a.txt", O_TRUNC|O_CREAT|O_SYNC);
		//int fd = open("/root/trunk_app/app.tar", O_RDONLY);
		int fd = open("./app.tar", O_RDONLY);
		if(fd < 0) {
			printf("open error\n");
			return;
		}
		int read_len = read(fd, buff+sizeof(msg_header_t), BUFF_MAX);
		if(read_len <= 0) {
			return;
		}
		close(fd);
		res_msg->len = read_len + sizeof(msg_header_t);
		res_msg->command = RES_DOWNLOAD_PACK;
		if(send(clnt->sock, buff, res_msg->len, 0) < 0) {
		//if(wsock_send(clnt, buff, res_msg->len) < 0) {
			printf(">> RES_DOWNLOAD_PACK (SENDING FAILED)\n", res_msg->len);
			return;
		}
		hexdump("send.dump", (unsigned char *)buff+sizeof(msg_header_t), read_len);
		printf(">> RES_DOWNLOAD_PACK (%d bytes)\n", res_msg->len);
	}
}










