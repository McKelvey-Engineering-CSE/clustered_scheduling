#ifndef PROCESS_PRIMITIVES_H
#define PROCESS_PRIMITIVES_H

#include <pthread.h>
#include <cerrno>
#include <iostream>

class p_mutex {

private: 

    // pthread_mutex_t mutex; 
    pthread_mutex_t mutex;
    pthread_mutexattr_t mutexAttr;

    pthread_cond_t cond; 
    pthread_condattr_t condAttr;

    // check if p_mutex has been initialized
    bool is_initialized;

public:

    p_mutex();
    ~p_mutex();

    
    // changed for robust mutex
    //////////////////////////////////
    // void lock();
    // void unlock();

    int lock();
    int unlock();
    ///////////////////////////////////

    void wait();
    void notify_all();

    bool is_valid() const;
    int trylock();
};

#endif
