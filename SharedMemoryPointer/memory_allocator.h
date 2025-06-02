#ifndef MEMORY_ALLOCATOR_H
#define MEMORY_ALLOCATOR_H
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <math.h>
#include <iostream>
#include <vector>
#include <bitset>

/*************************************************************************

memory_allocator.h

A template for constructing any object we want in a unified way. All objects
are memory mapped and then constructed in place before returning a pointer
to the object. 

The function takes in a class type for construction and it takes a memory 
segment name and any number of arguments which will be passed to the
constructor of the object for construction.

Functions: allocate <template> 

**************************************************************************/

namespace shared_memory_module{

    template <typename T>
    key_t nameToKey(T name){
        //take in the string and make sure it's less than 10 chars
        if (name.length() > 10){
            std::perror("ERROR: key names must be shorter than 10 characters\n");
            exit(-1);
        }

        std::string bits = "";

        //otherwise, make a number representing the string
        for (size_t i = 0; i < name.length(); i++){

            //if it's a digit
            if (name[i] >= 48 && name[i] <= 57){
                std::bitset<6> binaryRepresentation(name[i] - 48);
                bits += binaryRepresentation.to_string();
            }

            else if (name[i] >= 65 && name[i] <= 90){
                std::bitset<6> binaryRepresentation(name[i] - 55);
                bits += binaryRepresentation.to_string();
            }

            else if (name[i] >= 97 && name[i] <= 122){
                std::bitset<6> binaryRepresentation(name[i] - 61);
                bits += binaryRepresentation.to_string();
            }

            else if (name[i] == 95){
                std::bitset<6> binaryRepresentation(62);
                bits += binaryRepresentation.to_string();
            }

            else{
                std::perror("ERROR: key names must only contain alphanumeric values and underscores\n");
                exit(-1);
            }

        }
        return (key_t)(std::bitset<64>(bits).to_ullong());
    }

    template <class T, typename... Args>
    T* allocate(std::string bufferName, Args&&... args){

        T* object;

        int shmid = shmget(nameToKey<std::string>(bufferName), sizeof(T), IPC_CREAT | IPC_EXCL | 0666);
        void* memory_block = (T*)shmat(shmid, NULL, 0);
        object = new (memory_block) T{(std::forward<Args>(args))...};
            
        if (object == nullptr){
            std::perror("ERROR: opening allocated memory for printing call to shmget failed");
            return nullptr;
        }
        
        return object;
    }

    template <class T>
    T* fetch(std::string bufferName){

        T* object;
        
        int shmid = shmget(nameToKey<std::string>(bufferName), sizeof(T), 0666);
        
        // if (shmid == -1)
        // {
        //     std::error("The shmid is -1");
        // }

        object = (T*)shmat(shmid, NULL, 0);

        if (object == nullptr){
            std::perror("ERROR: fetching allocated memory for printing call to shmget failed");
            return nullptr;
        }

        // it's like the condition "(object == nullptr)"
        // will be false even when the shmid is -1, the
        // return value of object would be (T*)-1 instead
        if (object == (T*)-1)
        {
            return nullptr;
        }
        return object;
    }

    template <class T>
    int delete_memory(std::string bufferName){
        
        //remove barrier object
        return shmctl(shmget(shared_memory_module::nameToKey<std::string>(bufferName), sizeof(T), 0666), IPC_RMID, NULL);
    }

    template <class T>
    int detatch(T* mem_seg){
        
        //disconenct from memory
        return shmdt(mem_seg);
    }
} 

namespace smm = shared_memory_module;

#endif
