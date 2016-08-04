#include "global.h"

#include <iostream>

using namespace std;

char *g_buff = NULL;
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

int check_license()
{
	FILE *fp;
	//decrypt_fm(SEED, "./license.dat");
	return 0;
}

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

	g_buff = (char *)malloc(BUFF_MAX);
	if(g_buff == NULL) {
		printf("malloc() is failed.\n");
		return -1;
	}

	if(wsock_create_tcp_table(&table, 1, BUFF_MAX, 60) < 0) {
		printf("wsock_create_tcp_table() is failed.\n");
		return -1;
	}

	if((wsock = wsock_add_new_tcp_client(
			&table,
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
	pthread_create(&pid, NULL, f_runner, (void *) &table);

	check_license();
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

#ifndef DEBUG__
#define DEBUG__ printf("%s:%d\n", __func__, __LINE__)
#endif
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

void procedure(struct wsock *wsock)
{
	msg_header_t *msg = (msg_header_t *) g_buff;
	msg_param_t *param_info = (msg_param_t *) ((char *)g_buff + sizeof(msg_header_t));
	char *data = (char *) &param_info[1];
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
	msg_header_t *res_msg = (msg_header_t *)g_buff;
	int data_len = msg.len - sizeof(msg_header_t);

	char *data = (char *) (buff + sizeof(msg_header_t));

	if(msg.command == REQ_GET_LICENCE) {
	}
	else if(msg.command == REQ_GET_VERSION) {
	}
	if(msg.command == RES_DOWNLOAD_PACK) {
		printf(">> RES_DOWNLOAD_PACK\n");
		//int fd = open("/usr/local/app.tar", O_WRONLY|O_TRUNC|O_CREAT);
		int fd = open("./_app.tar", O_WRONLY|O_TRUNC|O_CREAT);//|O_DSYNC | O_RSYNC);
		if(fd < 0) {
			printf("open error\n");
			return;
		}
		write(fd, data, data_len);
		sync();
		close(fd);
		hexdump("recv.dump", (unsigned char *)data, data_len);

		//printf("extract files....\n");
		//system("tar -xvf /usr/local/app.tgz -C /usr/local/");
		//system("tar -xvf ./_app.tar"); 
		//printf("mod group/user id\n");
		//system("chown -R pi.pi /usr/local/app");
		sleep(1);
		exit(1);
	}
}

