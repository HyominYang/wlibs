/*
 * w_esocket.h
 *
 *  Created on: 2014. 12. 30.
 *      Author: wind
 */

#ifndef W_ESOCKET_H_
#define W_ESOCKET_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <syslog.h>

struct wsock;
struct wsock_table;

enum {
	WSOCK_NONE=0,
	WSOCK_TCP_SERVER,
	WSOCK_TCP_CLIENT,
	WSOCK_TCP_CLIENT_SINGLE
};

#define WSOCK_ADDR_IP_STRLEN_MAX 40
struct wsock_addr
{
	int flag_v6;
	union {
		struct sockaddr_in v4;
	};
	char ch_ip[WSOCK_ADDR_IP_STRLEN_MAX];
	unsigned short int h_port; ///< port : host order
};

struct wsock
{
	int sock;
	struct epoll_event ep_event;
	int type;
	struct wsock_addr addr_info;
	unsigned char *buff;
	int offset;
	int read_len;
	int buff_size;
	union wsock_data {
		struct {
			unsigned long int recv_bytes;
			unsigned long int sent_bytes;
			struct wsock *server;
		} client;
		struct {
			int client_count_current;
			int client_count_max;
		} server;
	} data;
	time_t time_begin;
	time_t time_end;

	void (*fn_connection) (struct wsock_table *, struct wsock *, struct wsock *);
	void (*fn_receive) (struct wsock_table *, struct wsock *, char *, int, int *);
	void (*fn_disconnection) (struct wsock_table *, struct wsock *);
	int flag_in_pool;
	struct wsock_table *table;
	struct wsock *prev;
	struct wsock *next;
};

#define WSOCK_TABLE_NAME_STRLEN_MAX 128
struct wsock_table
{
	char name[WSOCK_TABLE_NAME_STRLEN_MAX];
	pthread_spinlock_t lock;
	int epoll_fd;

	int flag_running;
	int flag_exit;
	int timeout;
	int element_count_current;
	int element_count_max;
	struct epoll_event *ep_event;
	struct wsock *wsock_mem;
	struct wsock wsock_pool;
	struct wsock wsock_head;
	unsigned char *buff_mem;
};

// Start Table
int wsock_table_run(struct wsock_table *table);

// Create Table
int wsock_create_tcp_table(struct wsock_table *table, int element_count, int buff_size, int timeout);

// Add New TCP Client
struct wsock *wsock_add_new_tcp_client(
		struct wsock_table *table, char *str_serv_addr, unsigned short int serv_port, unsigned short int clnt_port, int timeout, void *expand_data,
		void (*fn_receive) (struct wsock_table *, struct wsock *, char *, int, int *),
		void (*fn_disconnection) (struct wsock_table *, struct wsock *));

// Add New TCP Server
int wsock_add_new_tcp_server(
		struct wsock_table *table, char *str_addr, unsigned short int port, int backlog, int client_count, void *expand_data,
		void (*fn_connection) (struct wsock_table *, struct wsock *, struct wsock *),
		void (*fn_receive) (struct wsock_table *, struct wsock *, char *, int, int *),
		void (*fn_disconnection) (struct wsock_table *, struct wsock *));
// Clear wsock
void wsock_memset(struct wsock *wsock);

// Connection Release
int wsock_conn_release(struct wsock *wsock);

// Convert a String address to a Data address
int wsock_conv_str_address(char *str_addr, int str_addr_len, unsigned short int port, struct wsock_addr *serv_info);

// Client Connector
int wsock_connect_wait(int sockfd, struct sockaddr *saddr, int addrsize, int sec);

// Send Data
int wsock_send(struct wsock *wsock, unsigned char *buff, int len);
#endif /* W_ESOCKET_H_ */
