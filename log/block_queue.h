/*************************************************************
*循环数组实现的阻塞队列，m_back = (m_back + 1) % m_max_size;  
*线程安全，每个操作前都要先加互斥锁，操作完后，再解锁
**************************************************************/

#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "../lock/locker.h"
using namespace std;

template <class T>
class block_queue
{
public:
    block_queue(int max_size = 1000)
    {
        if (max_size <= 0)
        {
            exit(-1);
        }

        _max_size = max_size;
        _array = new T[max_size];
        _size = 0;
        _front = -1;
        _back = -1;
    }

    void clear()
    {
        MyLockGuard lk(_mutex);
        _size = 0;
        _front = -1;
        _back = -1;
    }

    ~block_queue()
    {
        MyLockGuard lk(_mutex);
        if (_array != NULL)
            delete [] _array;
    }
    //判断队列是否满了
    bool full() 
    {
        MyLockGuard lk(_mutex);
        if (_size >= _max_size)
        {
            return true;
        }
        return false;
    }
    //判断队列是否为空
    bool empty() 
    {
        MyLockGuard lk(_mutex);
        if (0 == _size)
        {
            return true;
        }
        return false;
    }
    //返回队首元素
    bool front(T &value) 
    {
        MyLockGuard lk(_mutex);
        if (0 == _size)
        {
            return false;
        }
        value = _array[_front];
        return true;
    }
    //返回队尾元素
    bool back(T &value) 
    {
        MyLockGuard lk(_mutex);
        if (0 == _size)
        {
            return false;
        }
        value = _array[_back];
        return true;
    }

    int size() 
    {
        int tmp = 0;

        MyLockGuard lk(_mutex);
        tmp = _size;

        return tmp;
    }

    int max_size()
    {
        int tmp = 0;

        MyLockGuard lk(_mutex);
        tmp = _max_size;

        return tmp;
    }
    //往队列添加元素，需要将所有使用队列的线程先唤醒
    //当有元素push进队列,相当于生产者生产了一个元素
    //若当前没有线程等待条件变量,则唤醒无意义
    bool push(const T &item)
    {
        MyLockGuard lk(_mutex);
        if (_size >= _max_size)
        {
            _cond.notify_all();
            return false;
        }

        _back = (_back + 1) % _max_size;
        _array[_back] = item;

        ++_size;

        _cond.notify_all();
        return true;
    }
    //pop时,如果当前队列没有元素,将会等待条件变量
    bool pop(T &item)
    {

        MyLockGuard lk(_mutex);
        while (_size <= 0)
        {
            
            if (!_cond.wait(_mutex.get()))
            {
                return false;
            }
        }

        _front = (_front + 1) % _max_size;
        item = _array[_front];
        --_size;
        return true;
    }

    //增加了超时处理
    bool pop(T &item, int ms_timeout)
    {
        struct timespec t = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now, NULL);
        MyLockGuard lk(_mutex);
        if (_size <= 0)
        {
            t.tv_sec = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;
            if (!_cond.time_wait(_mutex.get(), t))
            {
                return false;
            }
        }

        if (_size <= 0)
        {
            return false;
        }

        _front = (_front + 1) % _max_size;
        item = _array[_front];
        --_size;
        return true;
    }

private:
    MyMutex _mutex;
    MyCV _cond;

    T *_array;
    int _size;
    int _max_size;
    int _front;
    int _back;
};

#endif
