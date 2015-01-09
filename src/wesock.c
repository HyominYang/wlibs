/*
 * w_esocket.c
 *
 *  Created on: 2014. 12. 30.
 *      Author: wind
 */


#include "wesock.h"

#ifndef LINK_NULL
#define LINK_NULL -1
#endif

#ifdef LOG_HEAD
#undef LOG_HEAD
#endif

#define LOG_HEAD "[wsock:%4d] "
#define LOG_HEAD_PARAM __LINE__

#if 0
#define WDEBUG__ syslog(LOG_INFO, "%s:%d", __func__, __LINE__);
#else 
#define WDEBUG__
#endif

inline static void wsock_lock(struct wsock *wsock)
{
	pthread_spin_lock(&wsock->table->lock);
}
inline static void wsock_unlock(struct wsock *wsock)
{
	pthread_spin_unlock(&wsock->table->lock);
}
inline static void wsock_table_lock(struct wsock_table *table)
{
	pthread_spin_lock(&table->lock);
}
inline static void wsock_table_unlock(struct wsock_table *table)
{
	pthread_spin_unlock(&table->lock);
}

void wsock_release_tcp_table(struct wsock_table *table, int element_count, int  buff_size, int timeout)
{
	table->flag_exit = 1;
	while(table->flag_running == 1) {
		usleep(10000);
	}
}

int wsock_create_tcp_table(struct wsock_table *table, int element_count, int  buff_size, int timeout)
{
	int i, q_size;
	char str_error[256];

	memset(table, 0, sizeof(struct wsock_table));
	if(pthread_spin_init(&table->lock, 0)) {
		syslog(LOG_INFO, LOG_HEAD "pthread_spin_init() is failed.", LOG_HEAD_PARAM);
		return -1;
	}

	if((table->epoll_fd = epoll_create(element_count)) < 0) {
		strerror_r(errno, str_error, 256);
		syslog(LOG_INFO, LOG_HEAD "Error : %s", LOG_HEAD_PARAM, str_error);
		goto error;
	}

	q_size = element_count+1;

	table->ep_event = (struct epoll_event *) malloc(sizeof(struct epoll_event) * element_count);
	table->wsock_mem = (struct wsock *) malloc(sizeof(struct wsock) * element_count);
	table->buff_mem = (unsigned char *) malloc(buff_size * element_count);
	table->wsock_release_q = (struct wsock **) malloc(sizeof(struct wsock *) * q_size);
	if(table->ep_event == NULL || table->wsock_mem == NULL || table->buff_mem == NULL || table->wsock_release_q == NULL) {
		syslog(LOG_INFO, LOG_HEAD "malloc() is failed.", LOG_HEAD_PARAM);
		goto error;
	}

	memset(table->ep_event, 0, sizeof(struct epoll_event) * element_count);
	memset(table->wsock_mem, 0, sizeof(struct wsock) * element_count);
	memset(table->buff_mem, 0, buff_size * element_count);
	memset(table->wsock_release_q, 0, sizeof(struct wsock **) * q_size);


	table->q_size = q_size; 
	table->element_count_max = element_count;
	table->timeout = timeout;
	table->pool_count = element_count;

	for(i=0; i<element_count; i++) {
		table->wsock_mem[i].next = table->wsock_pool.next;
		table->wsock_pool.next = &table->wsock_mem[i];
		table->wsock_mem[i].buff = (unsigned char *) (table->buff_mem + (buff_size * i));
		table->wsock_mem[i].buff_size = buff_size;
		table->wsock_pool.next->flag_in_pool = 1;
		table->wsock_pool.next->table = table;
	}

	return 0;

error:
	pthread_spin_destroy(&table->lock);
	if(table->ep_event != NULL) {
		free(table->ep_event);
	}
	if(table->wsock_mem != NULL) {
		free(table->wsock_mem);
	}
	if(table->buff_mem != NULL) {
		free(table->buff_mem);
	}

	return -1;
}

int wsock_conv_str_address(char *str_addr, int str_addr_len, unsigned short int port, struct wsock_addr *serv_info)
{
	int flag_v6, flag_num, i;

	for(i=0, flag_v6=0, flag_num=0; i<str_addr_len; i++) {
		if((str_addr[i] >= '0' && str_addr[i] <= '9') ||
				(str_addr[i] >= 'a' && str_addr[i] <= 'f') ||
				(str_addr[i] >= 'A' && str_addr[i] <= 'F')
				) {
			flag_num = 1;
		}
		else if(str_addr[i] == '.') {
			if(flag_num == 0) {
				return -1;
			}
			break;
		} else {
			if(str_addr[i] == ':') {
				if(flag_num == 0) {
					return -1;
				}
				flag_v6=1;
				break;
			}
		}
	}

	if(i == str_addr_len) {
		return -1;
	}

	if(flag_v6 == 1) {
		serv_info->flag_v6 = 1;
	} else {
		serv_info->flag_v6 = 0;
		inet_pton(AF_INET, str_addr, &serv_info->v4.sin_addr.s_addr);
		inet_ntop(AF_INET, &serv_info->v4.sin_addr.s_addr, serv_info->ch_ip, WSOCK_ADDR_IP_STRLEN_MAX);
		serv_info->h_port = port;
		serv_info->v4.sin_port = htons(port);
		serv_info->v4.sin_family = AF_INET;
	}

	return 0;
}

int wsock_connect_wait(int sockfd, struct sockaddr *saddr, int addrsize, int sec) 
{ 
	/*Copy from : http://www.joinc.co.kr/modules/moniwiki/wiki.php/Site/Network_Programing/Documents/Sockettimeout*/
	int newSockStat; 
	int orgSockStat; 
	int res, n; 
	fd_set  rset, wset; 
	struct timeval tval; 
	char str_error[256];

	int error = 0; 
	int esize; 

	if ( (newSockStat = fcntl(sockfd, F_GETFL, NULL)) < 0 )  
	{ 
		//perror("F_GETFL error"); 
		strerror_r(errno, str_error, 256);
		syslog(LOG_INFO, LOG_HEAD "Error : %s.", LOG_HEAD_PARAM, str_error);
		return -1; 
	} 

	orgSockStat = newSockStat; 
	newSockStat |= O_NONBLOCK; 

	// Non blocking 상태로 만든다.  
	if(fcntl(sockfd, F_SETFL, newSockStat) < 0) 
	{ 
		//perror("F_SETLF error"); 
		strerror_r(errno, str_error, 256);
		syslog(LOG_INFO, LOG_HEAD "Error : F_SETLF - %s.", LOG_HEAD_PARAM, str_error);
		return -1; 
	} 

	// 연결을 기다린다. 
	// Non blocking 상태이므로 바로 리턴한다. 
	if((res = connect(sockfd, saddr, addrsize)) < 0) 
	{ 
		if (errno != EINPROGRESS) {
			syslog(LOG_INFO, LOG_HEAD "Error : connect() - EINPROGRESS", LOG_HEAD_PARAM);
			return -1; 
		}
	} 

	//printf("RES : %d\n", res); 
	// 즉시 연결이 성공했을 경우 소켓을 원래 상태로 되돌리고 리턴한다.  
	if (res == 0) 
	{ 
		//printf("Connect Success\n"); 
		syslog(LOG_INFO, LOG_HEAD "Connection is completed.", LOG_HEAD_PARAM);
		fcntl(sockfd, F_SETFL, orgSockStat); 
		return 1; 
	} 

	FD_ZERO(&rset); 
	FD_SET(sockfd, &rset); 
	wset = rset; 

	tval.tv_sec        = sec;     
	tval.tv_usec    = 0; 

	if ( (n = select(sockfd+1, &rset, &wset, NULL, &tval)) == 0) 
	{ 
		// timeout 
		errno = ETIMEDOUT;     
		syslog(LOG_INFO, LOG_HEAD "Try to connect is timeout.", LOG_HEAD_PARAM);
		return -1; 
	} 

	// 읽거나 쓴 데이터가 있는지 검사한다.  
	if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset) ) 
	{ 
		//printf("Read data\n"); 
		esize = sizeof(int); 
		if ((n = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&esize)) < 0)  {
			syslog(LOG_INFO, LOG_HEAD "Error : getsockopt()", LOG_HEAD_PARAM);
			return -1; 
		}
	} 
	else 
	{ 
		//perror("Socket Not Set"); 
		strerror_r(errno, str_error, 256);
		syslog(LOG_INFO, LOG_HEAD "Error : Socket Not Set(%s)", LOG_HEAD_PARAM, str_error);
		return -1; 
	} 


	fcntl(sockfd, F_SETFL, orgSockStat); 
	if(error) 
	{ 
		errno = error; 
		strerror_r(errno, str_error, 256);
		syslog(LOG_INFO, LOG_HEAD "Error : %s", LOG_HEAD_PARAM, str_error);
		return -1; 
	} 

	return 1; 
}

static void wsock_add_element(struct wsock *wsock)
{
	struct wsock_table *table = wsock->table;

	wsock->next = table->wsock_head.next;

	if(table->wsock_head.next != NULL) {
		table->wsock_head.next->prev = wsock;
	}
	table->wsock_head.next = wsock;
}

static struct wsock *wsock_get_element(struct wsock_table *table)
{
	struct wsock *wsock;
	
	if(table->wsock_pool.next == NULL) {
		return NULL;
	}

	table->pool_count--;
	wsock = table->wsock_pool.next;
	table->wsock_pool.next = wsock->next;

	return wsock;
}

static void wsock_release_element(struct wsock *wsock)
{
	struct wsock_table *table = wsock->table;

	wsock->flag_in_pool = 1;
	wsock->next = table->wsock_pool.next;
	table->wsock_pool.next = wsock;
	table->pool_count++;
}

struct wsock *wsock_add_new_tcp_client(
		struct wsock_table *table, char *str_serv_addr, unsigned short int serv_port, unsigned short int clnt_port, int timeout, void *expand_data,
		void (*fn_receive) (struct wsock_table *, struct wsock *, unsigned char *, int, int *),
		void (*fn_disconnection) (struct wsock_table *, struct wsock *))
{
	int sock, tmp;
	struct epoll_event ep_event;
	struct wsock *lp_wsock;
	char *str_local_ip_v4="127.0.0.1";
	struct wsock_addr serv_info;
	struct wsock_addr clnt_info;
	char str_error[256];

	if(table == NULL) {
		syslog(LOG_INFO, LOG_HEAD "table is not init.", LOG_HEAD_PARAM);
		return NULL;
	}

	memset(&serv_info, 0, sizeof(serv_info));
	memset(&clnt_info, 0, sizeof(clnt_info));
	if(wsock_conv_str_address(str_serv_addr, strlen(str_serv_addr), serv_port, &serv_info) < 0) {
		syslog(LOG_INFO, LOG_HEAD "address(%s) is wrong.", LOG_HEAD_PARAM, str_serv_addr);
		return NULL;
	}

	wsock_table_lock(table);
	if(table->element_count_max - table->element_count_current <= 0) {
		wsock_table_unlock(table);
		syslog(LOG_INFO, LOG_HEAD "Empty-slot has none.", LOG_HEAD_PARAM);
		return NULL;
	}

	if(serv_info.flag_v6 == 1) {
		wsock_table_unlock(table);
		syslog(LOG_INFO, LOG_HEAD "IPv6 Service don't support yet.", LOG_HEAD_PARAM);
		return NULL;
	} else {
		clnt_info.flag_v6 = 0;
		wsock_conv_str_address(str_local_ip_v4, strlen(str_local_ip_v4), clnt_port, &clnt_info);
		if((sock = socket(serv_info.v4.sin_family, SOCK_STREAM, 0)) < 0) {
			wsock_table_unlock(table);
			strerror_r(errno, str_error, 256);
			syslog(LOG_INFO, LOG_HEAD "%s", LOG_HEAD_PARAM, str_error);
			return NULL;
		}

		tmp = 1;
		if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp)) < 0) {
			wsock_table_unlock(table);
			strerror_r(errno, str_error, 256);
			syslog(LOG_INFO, LOG_HEAD "%s", LOG_HEAD_PARAM, str_error);
			close(sock);
			return NULL;
		}

		if(clnt_port > 0) {
			if(bind(sock, (struct sockaddr *) &clnt_info.v4, sizeof(clnt_info.v4)) < 0) {
				wsock_table_unlock(table);
				strerror_r(errno, str_error, 256);
				syslog(LOG_INFO, LOG_HEAD "%s", LOG_HEAD_PARAM, str_error);
				close(sock);
				return NULL;
			}
		}

#if 0
		syslog(LOG_INFO, LOG_HEAD "sizeof(serv_info.v4) -> %ld, %ld", LOG_HEAD_PARAM, sizeof(serv_info.v4), sizeof(struct sockaddr_in));
		if(connect(sock, (struct sockaddr *) &serv_info.v4, sizeof(serv_info.v4)) < 0) {;
			strerror_r(errno, str_error, 256);
			wsock_table_unlock(table);
			close(sock);
			syslog(LOG_INFO, LOG_HEAD "Connect to Server(%s:%d) is failed(%s).", LOG_HEAD_PARAM, serv_info.ch_ip, serv_info.h_port, str_error);
			return NULL;
		}
#else
		if(wsock_connect_wait(sock, (struct sockaddr *) &serv_info.v4, sizeof(serv_info.v4), timeout) < 0) {
			wsock_table_unlock(table);
			close(sock);
			syslog(LOG_INFO, LOG_HEAD "Error: Connect to Server(%s:%d) is failed.(timeout : %d seconds)", LOG_HEAD_PARAM, serv_info.ch_ip, serv_info.h_port, timeout);
			return NULL;
		}
#endif

		fcntl(sock, F_SETFL, O_NONBLOCK); /* Change the socket into non-blocking state	*/

		lp_wsock = wsock_get_element(table);
		if(lp_wsock == NULL) {
			wsock_table_unlock(table);
			syslog(LOG_INFO, LOG_HEAD "Error: pool is empty.", LOG_HEAD_PARAM);
			close(sock);
			return NULL;
		}

		memset(&ep_event, 0, sizeof(ep_event));
		ep_event.events = EPOLLIN|EPOLLET;
		ep_event.data.ptr = (void *) lp_wsock;

		wsock_memset(lp_wsock);
		lp_wsock->sock = sock;
		lp_wsock->ep_event = ep_event;
		lp_wsock->type = WSOCK_TCP_CLIENT_SINGLE;
		lp_wsock->addr_info = clnt_info;
		lp_wsock->expand_data = expand_data;
		lp_wsock->fn_connection = NULL;
		lp_wsock->fn_disconnection = fn_disconnection;
		lp_wsock->fn_receive = fn_receive;

		if(epoll_ctl(table->epoll_fd, EPOLL_CTL_ADD, sock, &ep_event)) {
			wsock_release_element(lp_wsock);
			wsock_table_unlock(table);
			strerror_r(errno, str_error, 256);
			syslog(LOG_INFO, LOG_HEAD "%s", LOG_HEAD_PARAM, str_error);
			close(sock);
			return NULL;
		}

		wsock_add_element(lp_wsock);
		table->element_count_current++;

		wsock_table_unlock(table);
	}

	return lp_wsock;
}

int wsock_add_new_tcp_server(
		struct wsock_table *table, char *str_addr, unsigned short int port, int backlog, int client_count, void *expand_data,
		void (*fn_connection) (struct wsock_table *, struct wsock *, struct wsock *),
		void (*fn_receive) (struct wsock_table *, struct wsock *, unsigned char *, int, int *),
		void (*fn_disconnection) (struct wsock_table *, struct wsock *))
{
	int sock, tmp;
	struct epoll_event ep_event;
	struct wsock *lp_wsock;
	struct wsock_addr serv_info;
	char str_error[256];

	if(table == NULL) {
		syslog(LOG_INFO, LOG_HEAD "Error : table is not init.", LOG_HEAD_PARAM);
		return -1;
	}


	memset(&serv_info, 0, sizeof(serv_info));
	if(wsock_conv_str_address(str_addr, strlen(str_addr), port, &serv_info) < 0) {
		syslog(LOG_INFO, LOG_HEAD "Error : address(%s) is wrong.", LOG_HEAD_PARAM, str_addr);
		return -1;
	}

	wsock_table_lock(table);
	if((table->element_count_max - table->element_count_current) < client_count+1) {
		syslog(LOG_INFO, LOG_HEAD "Error : table slot count is less than requirement slots.(Left count : %d, Requirement count : %d +1 is Server itself)", LOG_HEAD_PARAM, table->element_count_max - table->element_count_current, client_count+1);
		wsock_table_unlock(table);
		return -1;
	}

	if(table->flag_running == 1) {
		wsock_table_unlock(table);
		syslog(LOG_INFO, LOG_HEAD "Error : add TCP Server is failed. table is running.", LOG_HEAD_PARAM);
		return -1;
	}

	if(serv_info.flag_v6 == 1) {
		wsock_table_unlock(table);
		syslog(LOG_INFO, LOG_HEAD "Error : IPv6 Service don't support yet.", LOG_HEAD_PARAM);
		return -1;
	} else {
		if((sock = socket(serv_info.v4.sin_family, SOCK_STREAM, 0)) < 0) {
			wsock_table_unlock(table);
			strerror_r(errno, str_error, 256);
			syslog(LOG_INFO, LOG_HEAD "Error : %s", LOG_HEAD_PARAM, str_error);
			return -1;
		}

		tmp = 1;
		if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp)) < 0) {
			wsock_table_unlock(table);
			strerror_r(errno, str_error, 256);
			syslog(LOG_INFO, LOG_HEAD "Error : %s", LOG_HEAD_PARAM, str_error);
			close(sock);
			return -1;
		}

		if(bind(sock, (struct sockaddr *) &serv_info.v4, sizeof(serv_info.v4)) < 0) {
			wsock_table_unlock(table);
			strerror_r(errno, str_error, 256);
			syslog(LOG_INFO, LOG_HEAD "Error : %s", LOG_HEAD_PARAM, str_error);
			close(sock);
			return -1;
		}

		if(listen(sock, backlog) < 0) {
			wsock_table_unlock(table);
			strerror_r(errno, str_error, 256);
			syslog(LOG_INFO, LOG_HEAD "Error : %s", LOG_HEAD_PARAM, str_error);
			close(sock);
			return -1;
		}

		fcntl(sock, F_SETFL, O_NONBLOCK); /* Change the socket into non-blocking state	*/

		lp_wsock = wsock_get_element(table);
		if(lp_wsock == NULL) {
			wsock_table_unlock(table);
			syslog(LOG_INFO, LOG_HEAD "Error : pool is empty.", LOG_HEAD_PARAM);
			close(sock);
			return -1;
		}

		memset(&ep_event, 0, sizeof(ep_event));
		ep_event.events = EPOLLIN|EPOLLET;
		ep_event.data.ptr = (void *) lp_wsock;

		wsock_memset(lp_wsock);
		lp_wsock->sock = sock;
		lp_wsock->ep_event = ep_event;
		lp_wsock->type = WSOCK_TCP_SERVER;
		lp_wsock->addr_info = serv_info;
		lp_wsock->data.server.client_count_max = client_count;
		lp_wsock->expand_data = expand_data;
		lp_wsock->fn_connection = fn_connection;
		lp_wsock->fn_disconnection = fn_disconnection;
		lp_wsock->fn_receive = fn_receive;

		if(epoll_ctl(table->epoll_fd, EPOLL_CTL_ADD, sock, &ep_event)) {
			wsock_release_element(lp_wsock);
			wsock_table_unlock(table);
			strerror_r(errno, str_error, 256);
			syslog(LOG_INFO, LOG_HEAD "%s", LOG_HEAD_PARAM, str_error);
			close(sock);
			return -1;
		}

		wsock_add_element(lp_wsock);

		table->element_count_current++;

		wsock_table_unlock(table);
	}

	return 0;
}

int wsock_table_run(struct wsock_table *table)
{
	int sock, event_count, i, front, rear, sock_len;
	sigset_t sigs;
	int recv_ret;
	struct wsock *lp_element;
	struct wsock_addr sock_info;
	struct epoll_event ep_event;
	struct wsock *lp_wsock;
	char str_error[256];

	wsock_table_lock(table);
	table->flag_running = 1;
	wsock_table_unlock(table);

	sigfillset(&sigs);

	while(table->flag_exit == 0) {
		for(front=table->q_front, rear=table->q_rear; front != rear; front = (front+1)%table->q_size) {
			lp_element = table->wsock_release_q[front];
			if(lp_element->fn_disconnection != NULL) {
				lp_element->fn_disconnection(table, lp_element);
			}
			lp_element->next = table->wsock_pool.next;
			table->wsock_pool.next = lp_element;
		}
		table->q_front = front;

		event_count = epoll_pwait(table->epoll_fd,
								table->ep_event,
								table->element_count_max,
								table->timeout,
								&sigs);

		if(event_count == 0) { // event do not occurred
			usleep(1000);
			continue;
		}

		if(event_count < 0) {
			strerror_r(errno, str_error, 256);
			syslog(LOG_INFO, LOG_HEAD "epoll_pwait() error : %s", LOG_HEAD_PARAM, str_error);
			continue;
		}

		for(i=0; i<event_count; i++) {
			lp_element = (struct wsock *) table->ep_event[i].data.ptr;
			// Event Check
			if(lp_element->type == WSOCK_TCP_SERVER) {
				if(lp_element->addr_info.flag_v6 == 0) {
					sock_len = sizeof(sock_info.v4);
					if((sock = accept(lp_element->sock, (struct sockaddr *) &sock_info.v4, (socklen_t *)&sock_len)) < 0) {
						strerror_r(errno, str_error, 256);
						syslog(LOG_INFO, LOG_HEAD "%s", LOG_HEAD_PARAM, str_error);
						continue;
					}
					wsock_table_lock(table);
					lp_wsock = wsock_get_element(table);
					wsock_table_unlock(table);
					if(lp_element->data.server.client_count_current ==
							lp_element->data.server.client_count_max || lp_wsock == NULL) {
						syslog(LOG_INFO, LOG_HEAD "Client slot is full(max : %d)", LOG_HEAD_PARAM, lp_element->data.server.client_count_max);
						close(sock);
						continue;
					}

					sock_info.flag_v6 = 0;
					sock_info.h_port = ntohs(sock_info.v4.sin_port);
					inet_ntop(AF_INET, &sock_info.v4.sin_addr.s_addr, sock_info.ch_ip, WSOCK_ADDR_IP_STRLEN_MAX);
					memset(&ep_event, 0, sizeof(ep_event));
					ep_event.events = EPOLLIN|EPOLLET;
					fcntl(sock, F_SETFL, O_NONBLOCK); // Sock Opt change : Non-Blocking mode

					ep_event.data.ptr = (void *) lp_wsock;
					if(epoll_ctl(table->epoll_fd, EPOLL_CTL_ADD, sock, &ep_event)) {
						wsock_table_lock(table);
						wsock_release_element(lp_wsock);
						wsock_table_unlock(table);
						strerror_r(errno, str_error, 256);
						syslog(LOG_INFO, LOG_HEAD "Add a client is failed(%s).", LOG_HEAD_PARAM, str_error);
						close(sock);
						table->flag_exit = 1;
						break;
					}

					lp_element->data.server.client_count_current++;
					table->element_count_current++;

					wsock_memset(lp_wsock);
					lp_wsock->type = WSOCK_TCP_CLIENT;
					lp_wsock->sock = sock;
					lp_wsock->addr_info = sock_info;
					lp_wsock->ep_event = ep_event;
					lp_wsock->data.client.server = lp_element;
					lp_wsock->fn_connection = lp_element->fn_connection;
					lp_wsock->fn_receive = lp_element->fn_receive;
					lp_wsock->fn_disconnection = lp_element->fn_disconnection;

					wsock_table_lock(table);
					wsock_add_element(lp_wsock);
					wsock_table_unlock(table);

					if(lp_wsock->fn_connection != NULL) {
						lp_wsock->fn_connection(table, lp_element, lp_wsock);
					}
				} // IPv4 Processing
			}
			else { // Client
receive_from_client:
				if(lp_element->addr_info.flag_v6 == 0) {
					// Receive Data from element(Client)
					wsock_table_lock(table);
					if(lp_element->flag_in_pool == 1) {
						wsock_table_unlock(table);
						continue;
					}
					recv_ret = recv(lp_element->sock, lp_element->buff + lp_element->offset, lp_element->buff_size, 0);
					wsock_table_unlock(table);
					strerror_r(errno, str_error, 256);
					syslog(LOG_INFO, LOG_HEAD "[%s:%d] Data recieved(%d) : %s", LOG_HEAD_PARAM, lp_element->addr_info.ch_ip, lp_element->addr_info.h_port, recv_ret, str_error);

					// Parse event types
					if(recv_ret == 0) { // Disconnected by a Client
disconnection_client:
						syslog(LOG_INFO, LOG_HEAD "[%s:%d] Disconnecting...", LOG_HEAD_PARAM, lp_element->addr_info.ch_ip, lp_element->addr_info.h_port);
						wsock_table_lock(table);
						if(wsock_conn_release(lp_element) < 0) {
							wsock_table_unlock(table);
							break;
						}
						wsock_table_unlock(table);
						continue;
					}
					else if(recv_ret < 0) { // Socket error
						strerror_r(errno, str_error, 256);
						syslog(LOG_INFO, LOG_HEAD "[%s:%d] Data recieve error : %s. Go to disconnect with peer.", LOG_HEAD_PARAM, lp_element->addr_info.ch_ip, lp_element->addr_info.h_port, str_error);
						goto disconnection_client;
					} else { // Received data
						lp_element->read_len = recv_ret;
						lp_element->data.client.recv_bytes += recv_ret;

						if(lp_element->fn_receive != NULL) {
							lp_element->fn_receive(table, lp_element, lp_element->buff, lp_element->read_len, &lp_element->offset);
						}
						if(recv_ret == lp_element->buff_size) {
							syslog(LOG_INFO, LOG_HEAD "[%s:%d] Receive Continue", LOG_HEAD_PARAM, lp_element->addr_info.ch_ip, lp_element->addr_info.h_port);
							goto receive_from_client;
						}
					}
				}
			}
		}

	}

	table->flag_running = 0;

	// Close connected sockets
	close(table->epoll_fd);
	lp_element = table->wsock_head.next;
	while(lp_element != NULL) {
		close(lp_element->sock);
		lp_element = lp_element->next;
	}
	pthread_spin_destroy(&table->lock);
	free(table->wsock_mem);
	free(table->buff_mem);

	return 0;
}

void wsock_memset(struct wsock *wsock)
{
	wsock->sock = -1;
	memset(&wsock->ep_event, 0, sizeof(struct epoll_event));
	wsock->type = WSOCK_NONE;
	memset(&wsock->addr_info, 0, sizeof(struct wsock_addr));
	memset(wsock->buff, 0, wsock->buff_size);
	wsock->read_len = 0;
	memset(&wsock->data, 0, sizeof(union wsock_data));
	wsock->time_begin = 0;
	wsock->time_end = 0;
	wsock->expand_data = NULL;
	wsock->user_data1 = NULL;
	wsock->user_data2 = NULL;
	wsock->fn_connection = NULL;
	wsock->fn_receive = NULL;
	wsock->fn_disconnection = NULL;
	wsock->flag_in_pool = 0;
	wsock->prev = NULL;
	wsock->next = NULL;
}

int wsock_conn_release(struct wsock *wsock)
{
	if(wsock->flag_in_pool == 0) {
		struct wsock_table *table;
		char str_error[256];

		wsock->flag_in_pool = 1;
		table = wsock->table;
		if(epoll_ctl(table->epoll_fd, EPOLL_CTL_DEL, wsock->sock, &wsock->ep_event)) {
			wsock_table_unlock(table);
			strerror_r(errno, str_error, 256);
			syslog(LOG_INFO, LOG_HEAD "[%s:%d] Disconnecting : Element delete error : %s", LOG_HEAD_PARAM, wsock->addr_info.ch_ip, wsock->addr_info.h_port, str_error);
			table->flag_exit = 1;
			return -1;
		}

		if(wsock->next != NULL) {
			wsock->next->prev = wsock->prev;
		}
		if(wsock->prev != NULL) {
			wsock->prev->next = wsock->next;
		} else {
			table->wsock_head.next = wsock->next;
		}

		close(wsock->sock);

		table->wsock_release_q[table->q_rear] = wsock;
		table->q_rear = (table->q_rear + 1)%(table->q_size);

		// Count update
		table->element_count_current--; 
		switch(wsock->type) {
			case WSOCK_TCP_CLIENT:
				wsock->data.client.server->data.server.client_count_current--;
			case WSOCK_TCP_CLIENT_SINGLE:
				syslog(LOG_INFO, LOG_HEAD "[%s:%d] Send(%lu) Receive(%lu)", LOG_HEAD_PARAM, wsock->addr_info.ch_ip, wsock->addr_info.h_port, wsock->data.client.sent_bytes, wsock->data.client.recv_bytes);
				break;
		}
	} else {
		syslog(LOG_INFO, LOG_HEAD "Connection Release is failed. Disconnected already...", LOG_HEAD_PARAM);
	}

	return 0;
}

int wsock_send(struct wsock *wsock, unsigned char *buff, int len)
{
	int wrote_len, offset, left_len;
	static char err_buff[512];

	if(len <= 0) {
		syslog(LOG_INFO, LOG_HEAD "Sending data size is zero.", LOG_HEAD_PARAM);
		return -1;
	}

	offset = 0;
	left_len = len;
	do {
		wsock_table_lock(wsock->table);
		if(wsock->flag_in_pool == 1) {
			syslog(LOG_INFO, LOG_HEAD "Sending data is stopped. connection is released.", LOG_HEAD_PARAM);
			wsock_table_unlock(wsock->table);
			break;
		}
		wrote_len = send(wsock->sock, buff, left_len, MSG_DONTWAIT);
		wsock_table_unlock(wsock->table);
		if(wrote_len == left_len) {
			wsock->data.client.sent_bytes += wrote_len;
			break;
		}
		else if(wrote_len < 0) {
			if(errno == EAGAIN) {
				usleep(1000);
				continue;
			}
			strerror_r(errno, err_buff, 512);
			syslog(LOG_INFO, LOG_HEAD "Send data is error : %s", LOG_HEAD_PARAM, err_buff);
			wsock_table_lock(wsock->table);
			wsock_conn_release(wsock);
			wsock_table_unlock(wsock->table);
			return wrote_len;
		} else {
			wsock->data.client.sent_bytes += wrote_len;
			offset += wrote_len;
			left_len -= wrote_len;
			usleep(1);
		}
	} while(1);

	return len;
}

