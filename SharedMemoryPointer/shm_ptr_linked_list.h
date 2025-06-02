#ifndef SHARED_PTR_LINKED_LIST_H
#define SHARED_PTR_LINKED_LIST_H

#include <functional>
#include <iostream>
#include <cassert>
#include <csignal>
#include <optional>
// #include "shm_ptr.h"

/*

During the execution:

Head -> (ptr(int), act_1) -> (ptr(char), act_2) -> nullptr
                   ---------------->
* Adding the node according by the order of declarations.



Before exits:

Head -> (ptr(int), act_1) -> (ptr(char), act_2) -> nullptr
                   <----------------
* Cleaning up in the reverse order.


*/

template <typename T>
class shared_mem_ptr;

void default_cleanup()
{
    // default cleanup function
    std::cout << "Default cleanup function executed." << std::endl;
}

struct BaseNode
{
    std::function<void()> cleanup_function; // points to the cleanup function for the underlying part
    BaseNode* next;
    BaseNode* prev;

    BaseNode(std::function<void()> cleanup_func)
        : cleanup_function(cleanup_func), next(nullptr), prev(nullptr) {}

    virtual ~BaseNode() {}
    virtual void cleanup_shared_memory() = 0;
    virtual void get_name() = 0;
};

template <typename T>
struct Node : public BaseNode {
    shared_mem_ptr<T>* shm_ptr;

    Node(shared_mem_ptr<T>* ptr, std::function<void()> cleanup_func)
        : BaseNode(cleanup_func), shm_ptr(ptr) {}
    
    void cleanup_shared_memory() override
    {
        if (shm_ptr)
        {
            shm_ptr->cleanup();
        }
    }

    void get_name() override
    {
        if (shm_ptr)
        {
            std::cout << "node: " << shm_ptr->get_name() << std::endl;
        }
    }
};

// bidirectional linked list
class SharedMemLinkedList {
public:
    BaseNode* head;
    BaseNode* tail;
    static SharedMemLinkedList* instance;

public:
    SharedMemLinkedList() : head(nullptr), tail(nullptr) {
        instance = this;
        register_cleanup();
    }

    ~SharedMemLinkedList() {
        clear();
    }

    // add new shared_mem_ptr and its cleanup function
    template <typename T>
    void add(shared_mem_ptr<T>* ptr, std::optional<std::function<void()>> cleanup_func = std::nullopt) {

        // if no cleanup function passed, assigned with the default one
        std::function<void()> func = cleanup_func.value_or(default_cleanup);
        
        BaseNode* new_node = new Node(ptr, func);
        new_node->get_name();
        // std::cout << "add node:" << new_node->get_name() << std::endl;

        if (!head) { // if the linked list is empty, make it the head
            head = tail = new_node;
        } else {     // if the linked list is not empty, append it to the tail
            tail->next = new_node;
            new_node->prev = tail;
            tail = new_node;
        }

    }

    // cleanup in reverse
    void clear() {
        //tail->get_name();
        if (!tail)
        {
            std::cout << "tail is empty" << std::endl;
        }
        std::cout << "Begin to clean up the shared memory list..." << std::endl;
        BaseNode* current = tail;
        while (current) {
            std::cout << "-----------------" << std::endl;
            current->cleanup_shared_memory();
            std::cout << "shm_ptr cleanuped" << std::endl;
            if (current->cleanup_function) {
                current->cleanup_function();
            }
            std::cout << "A share memory pointer node fully removed from the process." << std::endl;
            BaseNode* temp = current;
            current = current->prev;
            delete temp;
        }
        head = tail = nullptr;
    }


    void cleanup_on_exit() {
        std::cerr << "Cleaning up shared memory pointers..." << std::endl;
        clear();
        std::cerr << "Cleanup completed." << std::endl;
    }

   static void static_cleanup_handler(int signal) {
        if (instance) {
            instance->cleanup_on_exit();
        }
        std::exit(signal);
    }

    void register_cleanup() {
        std::signal(SIGINT, static_cleanup_handler);
        std::signal(SIGTERM, static_cleanup_handler);
        std::signal(SIGSEGV, static_cleanup_handler);
        std::signal(SIGABRT, static_cleanup_handler);
    }


};




SharedMemLinkedList* SharedMemLinkedList::instance = nullptr;


#endif // SHARED_PTR_LINKED_LIST_H
