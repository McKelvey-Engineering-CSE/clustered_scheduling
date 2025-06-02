#include "process_primitives.h"

typedef enum {
    LOCKED,
    UNLOCKED,
    PREVOWNERDEAD
} LockState;

p_mutex::p_mutex() : is_initialized(false){

    //make process safe
    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);

    // make the mutex robust
    // make sure to unlock a robust mutex before cleaning up it
    pthread_mutexattr_setrobust(&mutexAttr, PTHREAD_MUTEX_ROBUST);

    pthread_mutex_init(&mutex, &mutexAttr);

    //make process safe
    pthread_condattr_init(&condAttr);
    pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&cond, &condAttr);


    //if the program successfully gets here,
    //just suppose it has been initialized
    is_initialized = true;
}

p_mutex::~p_mutex(){
    pthread_mutex_destroy(&mutex);
}

// void p_mutex::lock(){
//     pthread_mutex_lock(&mutex);
// }

int p_mutex::lock()
{
    int ret = pthread_mutex_lock(&mutex);
    if (ret == 0)
    {
        // std::cout << "Mutex locked successfully." << std::endl;
        return LOCKED;
    } else if (ret == EOWNERDEAD) {
        std::cerr << "Previous owner crashed. Recovering mutex..." << std::endl;
        pthread_mutex_consistent(&mutex); // the lock is recovered, but the inner part still needs to be recovered as well
        std::cout << "Mutex recovered." << std::endl;
        return PREVOWNERDEAD;
    } else {
        std::cerr << "Failed to acquire mutex. Error: " << ret << std::endl;
        return UNLOCKED;
    }
}

void p_mutex::wait(){

    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);

}

void p_mutex::notify_all(){
    pthread_cond_broadcast(&cond);
}

// void p_mutex::unlock(){
//     pthread_mutex_unlock(&mutex);
// }

int p_mutex::unlock(){
    return pthread_mutex_unlock(&mutex);
}

bool p_mutex::is_valid() const {
    return is_initialized;
}

int p_mutex::trylock()
{
    return pthread_mutex_trylock(&mutex);
}
