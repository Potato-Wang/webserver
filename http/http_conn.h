#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>

#include "../lock/locker.h"
#include "../mysql/sql_connection_pool.h"
#include "../timer/timer.h"
#include "../log/log.h"

class HttpConn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    HttpConn() {}
    ~HttpConn() {}

public:
    void init(int sockfd, const sockaddr_in &addr, char *, int, int, string user, string passwd, string sqlname);
    void close_conn(bool real_close = true);
    void process();
    bool read_once();
    bool write();
    sockaddr_in *get_address()
    {
        return &_address;
    }
    void initmysql_result(connection_pool *connPool);
    int timer_flag;
    int improv;

    static int epollfd;
    static int user_count;
    MYSQL *mysql;
    int state;  //读为0, 写为1
    int close_log;


private:
    void init();
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char *get_line() { return _read_buf + _start_line; };
    LINE_STATUS parse_line();
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

private:
    int _sockfd;
    sockaddr_in _address;
    char _read_buf[READ_BUFFER_SIZE];
    long _read_idx;
    long _checked_idx;
    int _start_line;
    char _write_buf[WRITE_BUFFER_SIZE];
    int _write_idx;
    CHECK_STATE _check_state;
    METHOD _method;
    char _real_file[FILENAME_LEN];
    char *_url;
    char *_version;
    char *_host;
    long _content_length;
    bool _linger;
    char *_file_address;
    struct stat _file_stat;
    struct iovec _iv[2];
    int _iv_count;
    int _cgi;        //是否启用的POST
    char *_string; //存储请求头数据
    int _bytes_to_send;
    int _bytes_have_send;
    char *_doc_root;

    map<string, string> _users;
    int _TRIGMode;

    char _sql_user[100];
    char _sql_passwd[100];
    char _sql_name[100];
};

#endif
