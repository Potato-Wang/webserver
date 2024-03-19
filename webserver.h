#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "thread_pool/thread_pool.h"
#include "http/http_conn.h"

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TIMESLOT = 5;             //最小超时单位

class WebServer
{
public:
    WebServer();
    ~WebServer();

    void init(int port , string user, string passWord, string databaseName,
              int log_write , int opt_linger, int trigmode, int sql_num,
              int thread_num, int close_log, int actor_model);

    void thread_pool();
    void sql_pool();
    void write_log();
    void trig_mode();
    void eventListen();
    void eventLoop();
    void timer(int connfd, struct sockaddr_in client_address);
    void adjust_timer(UtilTimer *timer);
    void deal_timer(UtilTimer *timer, int sockfd);
    bool dealclientdata();
    bool dealwithsignal(bool& timeout, bool& stop_server);
    void dealwithread(int sockfd);
    void dealwithwrite(int sockfd);

public:
    //基础
    int port;
    char *root;
    int log_write;
    int close_log;
    int actormodel;

    int pipefd[2];
    int epollfd;
    HttpConn *users;

    //数据库相关
    connection_pool *connPool;
    string user;         //登陆数据库用户名
    string passWord;     //登陆数据库密码
    string databaseName; //使用数据库名
    int sql_num;

    //线程池相关
    ThreadPool<HttpConn> *pool;
    int thread_num;

    //epoll_event相关
    epoll_event events[MAX_EVENT_NUMBER];

    int listenfd;
    int OPT_LINGER;
    int TRIGMode;
    int LISTENTrigmode;
    int CONNTrigmode;

    //定时器相关
    ClientData *users_timer;
    Utils utils;
};
#endif
