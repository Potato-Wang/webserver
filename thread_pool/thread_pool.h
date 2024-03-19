#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../mysql/sql_connection_pool.h"

template <typename T>
class ThreadPool
{
public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    ThreadPool(int actor_model, connection_pool *connPool, int thread_number = 8, int max_request = 10000);
    ~ThreadPool();
    bool append(T *request, int state);
    bool append_p(T *request);

private:
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
    static void worker(void *arg);
    void run();

private:
    int _thread_number;        //线程池中的线程数
    int _max_requests;         //请求队列中允许的最大请求数
    pthread_t *_threads;       //描述线程池的数组，其大小为m_thread_number
    std::list<T *> _workqueue; //请求队列
    MyMutex _queuelocker;       //保护请求队列的互斥锁
    MySemaphore _queuestat;            //是否有任务需要处理
    connection_pool *_connPool;  //数据库
    int _actor_model;          //模型切换
};
template <typename T>
ThreadPool<T>::ThreadPool( int actor_model, connection_pool *connPool, int thread_number, int max_requests) : m_actor_model(actor_model),m_thread_number(thread_number), m_max_requests(max_requests), m_threads(NULL),m_connPool(connPool)
{
    if (thread_number <= 0 || max_requests <= 0)
        throw std::exception();
    _threads = new pthread_t[_thread_number];
    if (!_threads)
        throw std::exception();
    //初始化时就detach掉了
    for (int i = 0; i < thread_number; ++i)
    {
        if (pthread_create(_threads + i, NULL, &ThreadPool::worker, this) != 0)
        {
            delete[] _threads;
            throw std::exception();
        }
        if (pthread_detach(_threads[i]))
        {
            delete[] _threads;
            throw std::exception();
        }
    }
}
template <typename T>
ThreadPool<T>::~ThreadPool()
{
    delete[] _threads;
}
template <typename T>
bool ThreadPool<T>::append(T *request, int state)
{
    {
        MyLockGuard lk(_queuelocker);
        if (_workqueue.size() >= _max_requests)
        {
            return false;
        }
        request->_state = state;
        _workqueue.push_back(request);
    }
    m_queuestat.post();
    return true;
}
template <typename T>
bool ThreadPool<T>::append_p(T *request)
{
    {
        MyLockGuard lk(_queuelocker);
        if (_workqueue.size() >= _max_requests)
        {
            return false;
        }
        _workqueue.push_back(request);
    }
    _queuestat.post();
    return true;
}
template <typename T>
void ThreadPool<T>::worker(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;
    pool->run();
    return pool;
}
template <typename T>
void ThreadPool<T>::run()
{
    while (true)
    {
        _queuestat.wait();
        {
            MyLockGuard lk(_queuelocker);
            if (_workqueue.empty())
            {
                continue;
            }
            T *request = _workqueue.front();
            _workqueue.pop_front();
        }
        if (request==nullptr)
            continue;
        if (1 == _actor_model)
        {
            if (0 == request->_state)
            {
                if (request->read_once())
                {
                    request->improv = 1;
                    connectionRAII mysqlcon(&request->mysql, _connPool);
                    request->process();
                }
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
            else
            {
                if (request->write())
                {
                    request->improv = 1;
                }
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        }
        else
        {
            connectionRAII mysqlcon(&request->mysql, _connPool);
            request->process();
        }
    }
}
#endif
