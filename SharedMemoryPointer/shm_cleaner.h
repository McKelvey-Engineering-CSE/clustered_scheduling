#ifndef CLEANUP_H
#define CLEANUP_H

#include "shm_ptr_linked_list.h"
#include <csignal>
#include <cstdlib>
#include <iostream>

// global instance of the shm linked list
static SharedMemLinkedList shared_mem_list;

// register new shm_ptr and its cleanup functions to the linked list
template <typename T>
void register_shared_ptr(shared_mem_ptr<T>* ptr, std::function<void()> cleanup_func) {
    std::cout << "cleaner:" << ptr->get_name() << std::endl;
    shared_mem_list.add(ptr, cleanup_func);
}

void cleanup_on_exit() {
    std::cerr << "Cleaning up shared memory pointers..." << std::endl;
    shared_mem_list.clear();
    std::cerr << "Cleanup completed." << std::endl;
}

void register_cleanup() {
    // std::atexit(cleanup_on_exit);   // normal exits

    //void register_optional_sig_functions(int signal, void (func)(int)){
    
    //if signal == 1
    //    passed_sig_int = func;

    //}

    std::signal(SIGINT, [](int) {   // Ctrl+C interruption
        cleanup_on_exit();
        // if (passed_sig_int != nullptr)
        //     passed_sig_int(1);
        std::exit(SIGINT);
    });
    std::signal(SIGTERM, [](int) {  // kill signals
        cleanup_on_exit();
        std::exit(SIGTERM);
    });
    std::signal(SIGSEGV, [](int) {  // segfault
        cleanup_on_exit();
        std::exit(SIGSEGV);
    });
}

#endif // CLEANUP_H
