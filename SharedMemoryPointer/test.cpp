#include "shm_ptr_linked_list.h"
// #include "shm_cleaner.h"
#include "shm_ptr.h"
#include <random>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

void foo()
{
    // empty cleanup function
}

void test_segfault()
{
    // std::cout << "Child Process " << id << " triggering segmentation fault.\n";
    std::cout << "testing segfault" << std::endl;
    int *ptr = nullptr;
    *ptr = 42;
}


void test_sigabrt()
{
    abort();
}

void test_sigint()
{
    raise(SIGINT);
}

void test_sigterm()
{
    while(true);
}

void child_function_deadlock(int id)
{
    SharedMemLinkedList shared_mem_list;

    shared_mem_ptr<int> shm_ptr1("int");
    shared_mem_list.add(&shm_ptr1, foo);
    
    if (id == 1)
    {
        sleep(2);
        shm_ptr1.test_deadlock();
    }
    else
    {
        sleep(5);
        std::cout << "now testing robustness" << std::endl;
        shm_ptr1.test_robust();
    }
}

void child_function(int id) {
    std::cout << "Child Process " << id << " (PID: " << getpid() << ") is running.\n";
    
    SharedMemLinkedList shared_mem_list;

    shared_mem_ptr<int> shm_ptr1("int", [] {
        std::cout << "cleanup!\n";
    });
    // register_shared_ptr(&shm_ptr1, foo);
    // std::cout << "cleaner:" << shm_ptr1.get_name() << std::endl;
    // shared_mem_list.add(&shm_ptr1, foo);

    shared_mem_ptr<bool> shm_ptr2("bool");
    // register_shared_ptr(&shm_ptr1, foo);
    // std::cout << "cleaner:" << shm_ptr1.get_name() << std::endl;
    // shared_mem_list.add(&shm_ptr2, foo);

    shared_mem_ptr<std::string> shm_ptr3("string");
    // register_shared_ptr(&shm_ptr1, foo);
    // std::cout << "cleaner:" << shm_ptr1.get_name() << std::endl;
    // shared_mem_list.add(&shm_ptr3, foo);
    
    std::random_device rd;  // 硬件随机数种子（如果可用）
    std::mt19937 gen(rd()); // Mersenne Twister 伪随机数生成器
    std::uniform_int_distribution<int> dist(1, 5); // 生成 [1, 100] 之间的整数
    int r = dist(gen);
    
    sleep(r); // 模拟子进程的工作

    // if (r == 1)
    // {
    //     test_segfault();
    // }
    // else if (r == 2)
    // {
    //     test_sigabrt();
    // }
    // else if (r == 3)
    // {
    //     test_sigint();
    // }

    std::cout << "Child Process " << id << " (PID: " << getpid() << ") cnt: " << shm_ptr1.get_cnt() << std::endl;

    std::cout << "Child Process " << id << " (PID: " << getpid() << ") is exiting.\n";
}

int test_multiproc()
{
    const int num_children = 3;
    pid_t pids[num_children];
    for (int i = 0; i < num_children; ++i) {
        pids[i] = fork();
        if (pids[i] < 0) {
            std::cerr << "Fork failed\n";
            return 1;
        } else if (pids[i] == 0) {
            child_function(i);
            return 0;
        }
    }

    for (int i = 0; i < num_children; ++i) {
        waitpid(pids[i], nullptr, 0);
    }

    std::cout << "All child processes have completed.\n";
    return 0;
}

int main() {

    // SharedMemLinkedList shared_mem_list;
    // shared_mem_list.register_cleanup();

    // shared_mem_ptr<int> shm_ptr1("int");
    // // // register_shared_ptr(&shm_ptr1, foo);
    // // // std::cout << "cleaner:" << shm_ptr1.get_name() << std::endl;
    // shared_mem_list.add(&shm_ptr1, foo);

    // shm_ptr1.test_deadlock();

    // shared_mem_ptr<bool> shm_ptr2("bool");
    // shared_mem_list.add(&shm_ptr2, foo);


    // test_segfault();
    // test_segfault();


    test_multiproc();

    return 0;
}
