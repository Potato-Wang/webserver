#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class MySemaphore
{
public:
    MySemaphore()
    {
        if (sem_init(&_semaphore, 0, 0) != 0)
        {
            throw std::exception();
        }
    }
    MySemaphore(int val)
    {
        if (sem_init(&_semaphore, 0, val))
        {
            throw std::exception();
        }
    }
    ~MySemaphore()
    {
        sem_destroy(&_semaphore);
    }
    bool wait()
    {
        return sem_wait(&_semaphore) == 0;
    }
    bool post()
    {
        return sem_post(&_semaphore) == 0;
    }

private:
    sem_t _semaphore;
};

class MyMutex{
public:
    MyMutex(){
        if(pthread_mutex_init(&_mutex, NULL)!=0){
            throw std::exception();
        }
    }
    ~MyMutex(){
        pthread_mutex_destroy(&_mutex);
    }
    bool lock(){
        return pthread_mutex_lock(&_mutex)==0;
    }
    bool unlock(){
        return pthread_mutex_unlock(&_mutex)==0;
    }
    pthread_mutex_t* get(){
        return &_mutex;
    }
private:
    pthread_mutex_t _mutex;

};

class MyLockGuard{
public:
    MyLockGuard(MyMutex& mtx):_p_mtx(&mtx){
        _p_mtx->lock();
    }
    ~MyLockGuard(){
        _p_mtx->unlock();
    }
private:
    MyMutex* _p_mtx;
};

class MyCV{
public:
    MyCV(){
        if(pthread_cond_init(&_cv, NULL)!=0){
            throw std::exception();
        }
    }
    ~MyCV(){
        pthread_cond_destroy(&_cv);
    }
    bool wait(pthread_mutex_t* p_mtx){
        int ret = pthread_cond_wait(&_cv, p_mtx);
        return ret;
    }
    bool time_wait(pthread_mutex_t* p_mtx, struct timespec t){
        int ret = pthread_cond_timedwait(&_cv, p_mtx, &t);
        return ret;
    }
    bool notify_one(){
        return pthread_cond_signal(&_cv)==0;
    }
    bool notify_all(){
        return pthread_cond_broadcast(&_cv)==0;
    }
private:
    pthread_cond_t _cv;
};

#endif