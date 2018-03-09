#ifndef WESOCKET_HPP_
#define WESOCKET_HPP_

#include <iostream>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <queue>
#include <vector>

#include <signal.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

class wsock
{
public:
    enum SOCK_TYPE
    {
        SOCK_CLNT = 0,
        SOCK_SERV,
    };
    wsock()
    {
        m_logfd = stdout;
        init();
    }
    ~wsock()
    {
    }
    int get_type()
    {
        return m_type;
    }
    void set_type(int type)
    {
        m_type = type;
    }
    void set_nickname(const std::string &nick)
    {
        m_nick = nick;
    }
    int get_sock()
    {
        return m_sock;
    }
    void set_sock(const int sock, bool flag_v6 = false)
    {
        init();
        m_sock = sock;
        m_flag_v6 = flag_v6;
    }
    void set_sock(const int sock, int type, bool flag_v6 = false)
    {
        set_sock(sock, flag_v6);
        m_ts_begin = m_ts_update = m_ts_end = time(NULL);
        m_type = type;
    }
    bool is_v4()
    {
        return !m_flag_v6;
    }
    bool is_v6()
    {
        return m_flag_v6;
    }
    void set_info_v4(const struct sockaddr_in &info)
    {
        this->info.v4 = info;
    }
    void set_info_v6(const struct sockaddr_in6 &info)
    {
        this->info.v6 = info;
    }
    int recv_data(unsigned char *buff, size_t buff_size)
    {
        int read_len = recv(m_sock, buff, buff_size, 0);
        if(read_len == 0)
        {
            m_flag_closing = true;
        }
        if(read_len < 0)
        {
            m_flag_error = true;
            char err_string_buff[256];
            int err_string_buff_size = 256;
            strerror_r(errno, err_string_buff, err_string_buff_size);
            m_err_message = err_string_buff;
            std::fprintf(m_logfd, "%s\n", err_string_buff);

            switch(errno)
            {
            case EINTR:
                m_flag_closing = true;
                break;
            }
        }
        return read_len;
    }
    bool is_ended()
    {
        return m_flag_closing;
    }
    bool is_closed()
    {
        return m_flag_closed;
    }
    std::string get_error_message()
    {
        return m_err_message;
    }
    void set_timeout(const time_t t)
    {
        m_timeout = t;
    }
    bool is_timeout()
    {
        return (time(NULL) - m_ts_update > m_timeout) ? true : false;
    }
    void close_sock()
    {
        if(m_flag_closed == false)
        {
            close(m_sock);
            m_flag_closed = true;
        }
    }
    void set_logfd(FILE *fd)
    {
        m_logfd = fd;
    }
    void timestamp_update()
    {
        m_ts_update = time(NULL);
    }
    void timestamp_end()
    {
        m_ts_end = time(NULL);
    }
private:
    void init()
    {
        m_err_message = "";
        m_nick = "noname";
        m_flag_v6 = false;
        m_type = SOCK_CLNT;
        m_sock = -1;
        m_flag_closing = false;
        m_flag_closed = false;
        m_flag_error = false;
    }
    std::string m_err_message;
    std::string m_nick;
    bool m_flag_v6;
    bool m_flag_closing;
    bool m_flag_closed;
    bool m_flag_error;
    int m_read_len;
    int m_type;
    int m_sock;
    FILE *m_logfd;
    time_t m_timeout;
    time_t m_ts_begin;
    time_t m_ts_update;
    time_t m_ts_end;
    union
    {
        struct sockaddr_in v4;
        struct sockaddr_in6 v6;
    } info;
};

class wsock_observer
{
public:
    virtual void connected(const wsock *sock) {}
    virtual void ready_to_read(const wsock *sock) {}
    virtual void disconnected(const wsock *sock) {}
private:
};

class wsock_table
{
public:
    enum WSOCK_TABLE_RESULT
    {
        CREAT_OK = 0,
        CREAT_ERR,
        CREAT_ERR_EPOLL,
        CREAT_ERR_DUP,
        ADD_OK = 0,
        ADD_ERR,
        LISTEN_OK = 0,
        LISTEN_ERR_INIT,
    };

    void add_observer(wsock_observer *observer)
    {
        m_observer.push_back(observer);
    }

    int create(const int el_count, const int el_timeout_sec)
    {
        int result = CREAT_ERR;
        if(m_flag_init == true) {
            m_flag_init = false;
        }

        do
        {
            // initialize variables
            m_el_count = el_count;
            m_timeout = el_timeout_sec;

            // memory allocation
            m_events = new epoll_event [el_count];

            for(int i=0; i<el_count; i++) {
                wsock *sock = new wsock;
                m_sock_pool.push(sock);
            }

            // create descriptors
            if((m_epoll_fd = epoll_create(el_count)) < 0) {
                result = CREAT_ERR_EPOLL;
                break;
            }

            m_flag_init = true;
            result = CREAT_OK;
        } while (0);
        return result;
    }
    int add_server()
    {
        int result = ADD_ERR;
        result = ADD_OK;
        return result;
    }
    int listen()
    {
        if(m_flag_init == false)
        {
            return LISTEN_ERR_INIT;
        }

        int result = -1;
        int event_count;
        char err_string_buff[256];
        int err_string_buff_size = 256;
        sigset_t sigs;

        sigfillset(&sigs);
        m_flag_exit = false;
        while(m_flag_exit == false)
        {
            event_count = epoll_pwait(m_epoll_fd, m_events, m_el_count, 1000, &sigs);
            if(event_count == 0)
            {
                usleep(1000);
                continue;
            }
            else if(event_count < 0)
            {
                strerror_r(errno, err_string_buff, err_string_buff_size);
                std::fprintf(m_logfd, "%s\n", err_string_buff);
                m_flag_exit = true;
                continue;
            }

            for(int event_offset = 0; event_offset < m_el_count; event_offset++)
            {
                wsock *element = (wsock *) m_events[event_offset].data.ptr;
                if(element->get_type() == wsock::SOCK_SERV)
                {

                    if(element->is_v4())
                    {
                        struct sockaddr_in sock_info;
                        // get/assign a client socket
                        int sock_len = sizeof(sock_info);
                        int tmp_sock = accept(element->get_sock(),
                                         (struct sockaddr *) &sock_info, (socklen_t *) &sock_len);
                        if(tmp_sock < 0)
                        {
                            strerror_r(errno, err_string_buff, err_string_buff_size);
                            std::fprintf(m_logfd, "%s\n", err_string_buff);
                            continue;
                        }

                        // Get a socket class and assign this socket descriptor & sockaddr
                        wsock *sock = m_sock_pool.front();
                        m_sock_pool.pop();
                        if(m_sock_pool.empty() == true)
                        {
                            close(tmp_sock);
                            std::fprintf(m_logfd, "empty slot is zero.\n");
                            continue;
                        }

                        sock->set_sock(tmp_sock, wsock::SOCK_CLNT);
                        sock->set_info_v4(sock_info);

                        // Add a epoll event
                        struct epoll_event ep_event;
                        std::memset(&ep_event, 0, sizeof(ep_event));
                        ep_event.events = EPOLLIN | EPOLLET;
                        ep_event.data.ptr = sock;
                        if(epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, tmp_sock, &ep_event) != 0)
                        {
                            close(tmp_sock);
                            m_sock_pool.push(sock);
                            strerror_r(errno, err_string_buff, err_string_buff_size);
                            std::fprintf(m_logfd, "%s\n", err_string_buff);
                            m_flag_exit = true;
                            continue;
                        }

                        // Push a socket class into current list
                        m_sock_list.push_back(sock);
                        // Notify to observer(s)
                        for(unsigned int i=0; i<m_observer.size(); i++)
                        {
                            m_observer[i]->connected(sock);
                        }
                    }
                } // if(element->get_type() == wsock::SOCK_SERV)
                else if(element->get_type() == wsock::SOCK_CLNT)
                {
                    element->timestamp_update();
                    if(element->is_v4())
                    {
                        for(unsigned int i=0; i<m_observer.size(); i++)
                        {
                            m_observer[i]->ready_to_read(element);
                        }
                    }
                    if(element->is_ended() == true)
                    {
                        element->timestamp_end();
                        for(unsigned int i=0; i<m_observer.size(); i++)
                        {
                            m_observer[i]->disconnected(element);
                        }
                        element->close_sock();
                    }
                }
            }
        }
        return result;
    }
    void exit()
    {
        m_flag_exit = true;
    }
    void set_logfd(FILE *fd)
    {
        m_logfd = fd;
    }
    wsock_table()
    {
        m_logfd = stdout;
        m_epoll_fd = -1;
        m_flag_init = false;
        m_events = NULL;
        m_flag_exit = false;
    }
    ~wsock_table()
    {
        if(m_events != NULL) delete m_events;
    }
private:
    // element information
    FILE *m_logfd;
    int m_el_count;
    int m_timeout;
    bool m_flag_init;
    bool m_flag_exit;
    // epoll information
    int m_epoll_fd;
    struct epoll_event *m_events;
    std::vector<wsock_observer *> m_observer;
    std::vector<wsock*> m_sock_list;
    std::queue<wsock*> m_sock_pool;
    std::queue<wsock*> m_sock_close_wait;
};

void wsock_table_test()
{
    wsock_table t;

    t.create(10, 100);
}
#endif // WESOCKET_HPP_
