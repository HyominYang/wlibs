/*
 * w_esocket.c
 *
 *  Created on: 2014. 12. 30.
 *      Author: wind
 */


#include "w_esocket.h"

#ifndef LINK_NULL
#define LINK_NULL -1
#endif

#ifdef LOG_HEAD
#undef LOG_HEAD
#endif

#define LOG_HEAD "[wsock:%4d] "
#define LOG_HEAD_PARAM __LINE__

int wsock_create_tcp_table(struct wsock_table *table, int element_count, size_t buff_size, int timeout)
{
	int i;

	memset(table, 0, sizeof(struct wsock_table));
	if(pthread_spin_init(&table->lock, 0)) {
		syslog(LOG_NOTICE, LOG_HEAD "pthread_spin_init() is failed.", LOG_HEAD_PARAM);
		return -1;
	}

	if((table->epoll_fd = epoll_create(element_count)) < 0) {
		syslog(LOG_INFO, LOG_HEAD "%s", LOG_HEAD_PARAM, strerror(errno));
		goto error;
	}

	table->ep_event = (struct epoll_event *) malloc((struct epoll_event) * element_count);
	table->wsock_mem = (struct wsock *) malloc((struct wsock) * element_count);
	table->buff_mem = (unsigned char *) malloc(buff_size * element_count);
	if(table->ep_event == NULL || table->wsock_mem == NULL || table->buff_mem == NULL) {
		syslog(LOG_NOTICE, LOG_HEAD "malloc() is failed.", LOG_HEAD_PARAM);
		goto error;
	}

	memset(table->ep_event, 0, sizeof(struct epoll_event) * element_count);
	memset(table->wsock_mem, 0, sizeof(struct wsock) * element_count);
	memset(table->buff_mem, 0, buff_size * element_count);


	table->element_count_max = element_count;
	table->timeout = timeout;

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

int wsock_add_new_tcp_server(
		struct wsock_table *table, struct wsock_addr serv_info, int backlog, int client_count, void *expand_data,
		void (*fn_connection) (struct wsock_table *, struct wsock *, struct wsock *),
		void (*fn_receive) (struct wsock_table *, struct wsock *, char *, int, int *),
		void (*fn_disconnection) (struct wsock_table *, struct wsock *))
{
	int sock, tmp;
	struct epoll_event ep_event;
	struct wsock *lp_wsock;
	unsigned char *buff;
	unsigned int buff_size;

	if(table == NULL) {
		syslog(LOG_INFO, LOG_HEAD "table is not init.", LOG_HEAD_PARAM);
		return -1;
	}

	pthread_spin_lock(&table->lock);
	if(table->flag_running == 1) {
		pthread_spin_unlock(&table->lock);
		syslog(LOG_INFO, LOG_HEAD "table is not init.", LOG_HEAD_PARAM);
		return -1;
	}

	if(serv_info.flag_v6 == 1) {
		pthread_spin_unlock(&table->lock);
		syslog(LOG_INFO, LOG_HEAD "IPv6 Service don't support yet.", LOG_HEAD_PARAM);
		return -1;
	} else {
		if((sock = socket(serv_info.v4.sin_family, SOCK_STREAM, 0)) < 0) {
			pthread_spin_unlock(&table->lock);
			syslog(LOG_INFO, LOG_HEAD "%s", LOG_HEAD_PARAM, strerror(errno));
			return -1;
		}

		tmp = 1;
		if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp)) < 0) {
			pthread_spin_unlock(&table->lock);
			syslog(LOG_INFO, LOG_HEAD "%s", LOG_HEAD_PARAM, strerror(errno));
			close(sock);
			return -1;
		}

		if(bind(sock, (struct sockaddr *) &serv_info, sizeof(serv_info)) < 0) {
			pthread_spin_unlock(&table->lock);
			syslog(LOG_INFO, LOG_HEAD "%s", LOG_HEAD_PARAM, strerror(errno));
			close(sock);
			return -1;
		}

		if(listen(sock, backlog) < 0) {
			pthread_spin_unlock(&table->lock);
			syslog(LOG_INFO, LOG_HEAD "%s", LOG_HEAD_PARAM, strerror(errno));
			close(sock);
			return -1;
		}

		fcntl(sock, F_SETFL, O_NONBLOCK); /* Change the socket into non-blocking state	*/

		if((table->element_count_max - table->element_count_current) == 0) {
			pthread_spin_unlock(&table->lock);
			syslog(LOG_INFO, LOG_HEAD "pool is empty.", LOG_HEAD_PARAM);
			close(sock);
			return -1;
		}
		lp_wsock = table->wsock_pool.next;

		memset(&ep_event, 0, sizeof(ep_event));
		ep_event.events = EPOLLIN|EPOLLET;
		ep_event.data.ptr = (void *) lp_wsock;

		table->wsock_pool.next = lp_wsock->next;
		buff = lp_wsock->buff;
		buff_size = lp_wsock->buff_size;
		//memset(lp_wsock, 0, sizeof(struct wsock));
		wsock_memset(lp_wsock);
		lp_wsock->sock = sock;
		lp_wsock->ep_event = ep_event;
		lp_wsock->type = WSOCK_TCP_SERVER;
		lp_wsock->buff = buff;
		lp_wsock->buff_size = buff_size;
		lp_wsock->addr_info = serv_info;
		lp_wsock->data.server.client_count_max = client_count;
		lp_wsock->fn_connection = fn_connection;
		lp_wsock->fn_disconnection = fn_disconnection;
		lp_wsock->fn_receive = fn_receive;
		lp_wsock->next = table->wsock_head.next;

		if(epoll_ctl(table->epoll_fd, EPOLL_CTL_ADD, sock, &ep_event)) {
			lp_wsock->flag_in_pool = 1;
			lp_wsock->next = table->wsock_pool.next;
			table->wsock_pool.next = lp_wsock;
			pthread_spin_unlock(&table->lock);
			syslog(LOG_INFO, LOG_HEAD "%s", LOG_HEAD_PARAM, strerror(errno));
			close(sock);
			return -1;
		}
		if(table->wsock_head.next != NULL) {
			table->wsock_head.next->prev = lp_wsock;
		}
		table->wsock_head.next = lp_wsock;
		table->element_count_current++;

		pthread_spin_unlock(&table->lock);
	}

	return 0;
}

int wsock_table_run(struct wsock_table *table)
{
	int sock, event_count, i;
	unsigned char *buff;
	unsigned int buff_size;
	sigset_t sigs;
	int recv_ret;
	struct wsock *lp_element;
	struct wsock_addr sock_info;
	struct epoll_event ep_event;
	struct wsock *lp_wsock;

	pthread_spin_lock(&table->lock);
	table->flag_running = 1;
	pthread_spin_unlock(&table->lock);

	sigfillset(&sigs);

	while(table->flag_exit == 0) {
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
			syslog(LOG_NOTICE, LOG_HEAD "epoll_pwait() error : %s", LOG_HEAD_PARAM, strerror(errno));
			continue;
		}

		for(i=0; i<event_count; i++) {
			lp_element = (struct wsock *) table->ep_event[i].data.ptr;
			// Event Check
			if(lp_element->type == WSOCK_TCP_SERVER) {
				if(lp_element->addr_info.flag_v6 == 0) {
					if((sock = accept(lp_element->sock, (struct sockaddr *) &sock_info.v4, (socklen_t *)&sock_len)) < 0) {
						syslog(LOG_INFO, LOG_HEAD "%s", LOG_HEAD_PARAM, strerror(errno));
						continue;
					}
					if(lp_element->data.server.client_count_current ==
							lp_element->data.server.client_count_max) {
						syslog(LOG_INFO, LOG_HEAD "Client slot is full(max : %d)", LOG_HEAD_PARAM, lp_element->data.server.client_count_max);
						close(sock);
						continue;
					}

					sock_info.flag_v6 = 0;
					memset(&ep_event, 0, sizeof(ep_event));
					ep_event.events = EPOLLIN|EPOLLET;
					fcntl(sock, F_SETFL, O_NONBLOCK); // Sock Opt change : Non-Blocking mode

					pthread_spin_lock(&table->lock);
					lp_wsock = table->wsock_pool.next;
					ep_event.data.ptr = (void *) lp_wsock;
					if(epoll_ctl(table->epoll_fd, EPOLL_CTL_ADD, sock, &ep_event)) {
						pthread_spin_unlock(&table->lock);
						syslog(LOG_INFO, LOG_HEAD "Add a client is failed(%s).", LOG_HEAD_PARAM, strerror(errno));
						close(sock);
						table->flag_exit = 1;
						break;
					}

					lp_element->data.server.client_count_current++;
					table->element_count_current++;

					table->wsock_pool.next = lp_wsock->next;
					buff = lp_wsock->buff;
					buff_size = lp_wsock->buff_size;
					//memset(lp_wsock, 0, sizeof(struct wsock));
					wsock_memset(lp_wsock);
					lp_wsock->type = WSOCK_TCP_CLIENT;
					lp_wsock->sock = sock;
					lp_wsock->buff = buff;
					lp_wsock->buff_size = buff_size;
					lp_wsock->addr_info = sock_info;
					lp_wsock->ep_event = ep_event;
					lp_wsock->data.client.server = lp_element;
					lp_wsock->next = table->wsock_head.next;
					lp_wsock->fn_connection = lp_element->fn_connection;
					lp_wsock->fn_receive = lp_element->fn_receive;
					lp_wsock->fn_disconnection = lp_element->fn_disconnection;
					if(table->wsock_head.next != NULL) {
						table->wsock_head.next->prev = lp_wsock;
					}
					table->wsock_head.next = lp_wsock;

					pthread_spin_unlock(&table->lock);
					if(lp_wsock->fn_connection != NULL) {
						lp_wsock->fn_connection(table, lp_element, lp_wsock);
					}
				} // IPv4 Processing
			}
			else if(lp_element->type == WSOCK_TCP_CLIENT) {
				if(lp_element->addr_info.flag_v6 == 0) {
					// Receive Data from element(Client)
					pthread_spin_lock(&table->lock);
					if(lp_element->flag_in_pool == 1) {
						pthread_spin_unlock(&table->lock);
						continue;
					}
					recv_ret = recv(lp_element->sock, lp_element->buff + lp_element->offset, lp_element->buff_size, 0);
					pthread_spin_unlock(&table->lock);

					// Parse event types
					if(recv_ret == 0) { // Disconnected by a Client
disconnection_client:
						syslog(LOG_INFO, LOG_HEAD "[%s:%d] Disconnecting...", LOG_HEAD_PARAM, lp_element->addr_info.ch_ip, lp_element->addr_info.h_port);
						pthread_spin_lock(&table->lock);
						if(lp_element->flag_in_pool == 1) {
							pthread_spin_unlock(&table->lock);
							syslog(LOG_INFO, LOG_HEAD "[%s:%d] Disconnected already...", LOG_HEAD_PARAM, lp_element->addr_info.ch_ip, lp_element->addr_info.h_port);
							continue;
						}

						if(lp_element->fn_disconnection != NULL) {
							lp_element->fn_disconnection(table, lp_element);
						}
						if(epoll_ctl(table->epoll_fd, EPOLL_CTL_DEL, lp_element->sock, &lp_element->ep_event)) {
							pthread_spin_unlock(&table->lock);
							syslog(LOG_INFO, LOG_HEAD "[%s:%d] Disconnecting : Element delete error : %s", LOG_HEAD_PARAM, lp_element->addr_info.ch_ip, lp_element->addr_info.h_port, strerror(errno));
							table->flag_exit = 1;
							break;
						}
						close(lp_element->sock);

						if(lp_element->prev == NULL) {
							table->wsock_head.next = lp_element->next;
						} else {
							lp_element->prev->next = lp_element->next;
						}
						if(lp_element->next != NULL) {
							lp_element->next->prev = lp_element->prev;
						}

						lp_element->next = table->wsock_pool->next;
						table->wsock_pool->next = lp_element;
						lp_element->flag_in_pool = 1;

						pthread_spin_unlock(&table->lock);
						continue;
					}
					else if(recv_ret < 0) { // Socket error
						syslog(LOG_INFO, LOG_HEAD "[%s:%d] Data recieve error : %s. Go to disconnect with peer.", LOG_HEAD_PARAM, lp_element->addr_info.ch_ip, lp_element->addr_info.h_port, strerror(errno));
						goto disconnection_client;
					} else { // Received data
						lp_element->read_len = recv_ret;
						if(lp_element->fn_receive != NULL) {
							lp_element->fn_receive(table, lp_element, lp_element->buff, lp_element->read_len, &lp_element->offset);
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
	wsock->fn_connection = NULL;
	wsock->fn_receive = NULL;
	wsock->fn_disconnection = NULL;
	wsock->flag_in_pool = 0;
	wsock->prev = NULL;
	wsock->next = NULL;
}

void wsock_conn_release(struct wsock *wsock)
{
	if(wsock->falg_in_pool == 1) {
		pthread_spin_unlock(&wsock->table->lock);
		return ;
	}
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
		wrote_len = send(wsock->sock, buff, left_len, MSG_DONTWAIT);
		if(wrote_len == left_len) {
			break;
		}
		else if(wrote_len < 0) {
			strerror_r(errno, err_buff, 512);
			syslog(LOG_INFO, LOG_HEAD "Send data is error : %s", err_buff);
			break;
		} else {
			offset += wrote_len;
			left_len -= wrote_len;
			usleep(1);
		}
	} while(1);
}

int main(int argc, char **argv)
{
	return 0;
}
