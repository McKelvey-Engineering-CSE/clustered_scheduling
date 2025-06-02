#ifndef SHM_PTR
#define SHM_PTR

#include <iostream>
#include <csignal>
#include "memory_allocator.h"
#include "process_primitives.h"
// #include "shared_mem_names.h"
#include "shm_ptr_linked_list_1.h"

typedef enum {
    LOCKED,
    UNLOCKED,
    PREVOWNERDEAD
} LockState;

class SharedMemLinkedList;

struct inner_part
{
    int counter = 0;
    p_mutex r_mut;

    inner_part() : counter(0), r_mut() {
        // std::cout << "inner_part constructed with initialized p_mutex." << std::endl;
    }

    // this is a safe-lock
    void lock()
    {
        // trylock the mutex to see if a deadlock happens
        // maybe the process has locked it but for some reason it may lock this again
        int ret = trylock();
        if (ret == 0) {
            // the mutex has not been locked before, no need to lock here
            unlock(); // if not unlocked here where will be a ```DEADLOCK```
        } else if (ret == EBUSY) {
            // the process itself is a lock holder
            std::cout << "Deadlock detected." << std::endl;
            unlock();
        } else if (ret == EPERM) {
            // the mutex is locked by other processes, cannot do anything here
            std::cout << "Mutex is owned by another thread, cannot unlock." << std::endl;
        }

        // this is the real lock
        ret = r_mut.lock();

        // to handle inconsistency due to a lock holder getting killed
        if (ret == PREVOWNERDEAD)
        {
            // previous mutex owner is dead, thus the counter should -1
            std::cout << "the previous lock holder is dead" << std::endl;
            counter--;
        } else if (ret == UNLOCKED) {
            std::cout << "The mutex has failed to lock." << std::endl;
            return;
        } else {
            // the mutex has been locked
        }
        // std::cout << "Shared data has been locked." << std::endl;
    }

    void unlock()
    {
        int ret = r_mut.unlock();
    }

    int trylock()
    {
        int ret = r_mut.trylock();
        return ret;
    }
};

template <typename T>
class shared_mem_ptr
{
private:
    inner_part* inner = nullptr;
    T* underlying_object = nullptr;
    std::string name = "";

    // static void cleanup_wrapper() {
    //     if (shared_instance) {
    //         shared_instance->cleanup();
    //     }
    // }

    static shared_mem_ptr<T>* shared_instance;

public:
    using CleanupFn = std::function<void()>;

    void cleanup(){
        inner = smm::fetch<inner_part>(name + "_ct");

        //////////////////////////////////
        //lock the mutex
        // inner->r_mut.lock();
        inner->lock();
        //////////////////////////////////

        //indicate we are detached
        inner->counter--;
        //check if we are last process
        if ((inner->counter == 0)){

            std::cout << "Cleaning up " << name << std::endl;

            ////////////////////////
            // inner->r_mut.unlock();
            inner->unlock();
            ////////////////////////

            if (smm::detatch(inner) == -1)
            {
                std::cerr << "Failed to detach inner_part." << std::endl;
            }
            if (smm::detatch(underlying_object) == -1)
            {
                std::cerr << "Failed to detach inner_part." << std::endl;
            }
            if (smm::delete_memory<inner_part>(name + "_ct") == -1)
            {
                std::cerr << "Failed to delete shared memory." << std::endl;
            }
            if (smm::delete_memory<T>(name) == -1)
            {
                std::cerr << "Failed to delete shared memory." << std::endl;
            }

            // smm::delete_memory<inner_part>(name + "_ct");
            // smm::delete_memory<T>(name);
            std::cout << "Resources cleaned up." << std::endl;
        }
        //unlock mutex
        else
            ////////////////////////
            // inner->r_mut.unlock();
            inner->unlock(); // the lock is a robust mutex, if not unlocked here, the robust list (by kernel)
                             // will be destroyed, and a segfault is expected
            ////////////////////////
    }

    shared_mem_ptr(const std::string& obj_name, std::optional<CleanupFn> cleanup_fn = std::nullopt){
        
        if (obj_name.empty())
        {
            throw std::logic_error("Name cannot be empty.");
        }

        name = obj_name;
        // shared_instance = this;

        // std::signal(SIGINT, shm_ptr_signal_handler);
        // std::signal(SIGTERM, shm_ptr_signal_handler);
        // std::signal(SIGSEGV, shm_ptr_signal_handler);

        //fetch counter
        if ((inner = smm::fetch<inner_part>(name + "_ct")) == nullptr){
            // if (!memory_names->add_name(name + "_ct"))
            // {
            //     std::cerr << "Failed to add shared memory name to the memory names list (" << name + "_ct" << ")" << std::endl;
            // }
            // if (!memory_names->add_name(name))
            // {
            //     std::cerr << "Failed to add shared memory name to the memory names list (" << name << ")" << std::endl;
            // }
            smm::allocate<inner_part>(name + "_ct");
            smm::allocate<T>(name);
            inner = smm::fetch<inner_part>(name + "_ct");
        }
        else
        {
            std::cerr << "A share memory of name " << name << " has been created before." << std::endl;
        }

        if (!inner->r_mut.is_valid())
        {
            std::cerr << "Mutex is not properly initialized." << std::endl;
            throw std::runtime_error("Failed to initialize mutex in inner_part.");
        }

        //lock the mutex
        ////////////////////////
        // inner->r_mut.lock();
        inner->lock();
        ////////////////////////

        //indicate we are attached
        inner->counter++;

        // // notify that the an object has been attached
        // std::cout << "Object attached. Counter: " << inner->counter << std::endl;

        //unlock mutex
        ///////////////////////////
        // inner->r_mut.unlock();
        inner->unlock();
        ///////////////////////////

        //attach to real object
        underlying_object = smm::fetch<T>(name);
        //set exit protocol

        // std::cout << inner->counter << std::endl;

        // std::atexit(cleanup_wrapper);

        if (SharedMemLinkedList::instance == nullptr)
        {
            static SharedMemLinkedList auto_linked_list_initializer;
        }

        if (SharedMemLinkedList::instance) {
            SharedMemLinkedList::instance->add(this, cleanup_fn);
        }
    }

    ~shared_mem_ptr()
    {
        // cleanup();   // now the cleanup is invoked by the linked list
    }

    int get_cnt()
    {
        return inner->counter;
    }



    /*
    // for the safety, don't use the following things
    void set_cnt(int n)
    {
        inner->lock();
        inner->counter = n;
        inner->unlock();
    }

    void reset_cnt()
    {
        inner->lock();
        inner->counter = 1;
        inner->unlock();
    }

    void dec_cnt()
    {
        inner->lock();
        inner->counter -= 1;
        inner->unlock();
    }
    */

    void unlock()
    {
        inner->lock();
    }

    p_mutex get_mutex()
    {
        return inner->r_mut;
    }

    std::string get_name()
    {
        return name;
    }

    // only for testing
    void test_deadlock()
    {
        inner->lock();
        // sleep(2);
        std::cout << "Now testing deadlock." << std::endl;
        // abort();

        raise(SIGINT);

        // int *ptr = nullptr;
        // *ptr = 42;
        
        inner->unlock();
    }

    // only for testing
    void test_robust()
    {
        std::cout << "trying to get the lock" << std::endl;
        inner->lock();
        std::cout << "lock fixed" << std::endl;
        inner->unlock();
    }

    shared_mem_ptr(const shared_mem_ptr&) = delete;

    shared_mem_ptr& operator=(const shared_mem_ptr&) = delete;
};








// template <typename T>
// shared_mem_ptr<T>* shared_mem_ptr<T>::shared_instance = nullptr;



#endif // SHM_PTR
