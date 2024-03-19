#include "timer.h"
#include "../http/http_conn.h"

SortTimerList::SortTimerList()
{
    _head = nullptr;
    _tail = nullptr;
}
SortTimerList::~SortTimerList()
{
    UtilTimer *tmp = _head;
    while (tmp)
    {
        _head = tmp->next;
        delete tmp;
        tmp = _head;
    }
}

void SortTimerList::add_timer(UtilTimer *timer)
{
    if (timer == nullptr)
    {
        return;
    }
    if (_head == nullptr)
    {
        _head = _tail = timer;
        return;
    }
    if (timer->expire < _head->expire)
    {
        timer->next = _head;
        _head->prev = timer;
        _head = timer;
        return;
    }
    add_timer(timer, _head);
}
void SortTimerList::adjust_timer(UtilTimer *timer)
{
    if (timer == nullptr)
    {
        return;
    }
    UtilTimer *tmp = timer->next;
    if (tmp == nullptr || (timer->expire < tmp->expire))
    {
        return;
    }
    if (timer == _head)
    {
        _head = _head->next;
        _head->prev = NULL;
        timer->next = NULL;
        add_timer(timer, _head);
    }
    else
    {
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}
void SortTimerList::del_timer(UtilTimer *timer)
{
    if (timer == nullptr)
    {
        return;
    }
    if ((timer == _head) && (timer == _tail))
    {
        delete timer;
        _head = NULL;
        _tail = NULL;
        return;
    }
    if (timer == _head)
    {
        _head = _head->next;
        _head->prev = NULL;
        delete timer;
        return;
    }
    if (timer == _tail)
    {
        _tail = _tail->prev;
        _tail->next = NULL;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}
int SortTimerList::tick()
{
    if (_head == nullptr)
    {
        return -1;
    }

    time_t cur = time(NULL);
    UtilTimer *tmp = _head;
    while (tmp)
    {
        if (cur < tmp->expire)
        {
            return tmp->expire - cur + 1;
        }
        tmp->cb_func(tmp->user_data);
        _head = tmp->next;
        if (_head)
        {
            _head->prev = NULL;
        }
        delete tmp;
        tmp = _head;
    }
    return -1;
}

void SortTimerList::add_timer(UtilTimer *timer, UtilTimer *lst_head)
{
    UtilTimer *prev = lst_head;
    UtilTimer *tmp = prev->next;
    while (tmp)
    {
        if (timer->expire < tmp->expire)
        {
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    if (tmp == nullptr)
    {
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        _tail = timer;
    }
}

void Utils::init(int timeslot)
{
    _TIMESLOT = timeslot;
}

//对文件描述符设置非阻塞
int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//信号处理函数
void Utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    int ret = _timer_lst.tick();
    if (ret != -1)
    {
        _TIMESLOT = ret;
    }
    alarm(_TIMESLOT);
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::_pipefd = 0;
int Utils::_epollfd = 0;

class Utils;
void cb_func(ClientData *user_data)
{
    epoll_ctl(Utils::_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    --HttpConn::user_count;
}
