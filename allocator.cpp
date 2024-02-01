#ifndef _ALLOCATOR_CPP
#define _ALLOCATOR_CPP

#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <cstdio>

using namespace std;

pthread_mutex_t lock;

struct node {
    int id;
    int size;
    int idx;
    node* next;

    node(int id, int size, int idx, node* next) : id(id), size(size), idx(idx), next(next) {}
};

class HeapManager {
public:
    HeapManager() : head(nullptr) {}

    int initHeap(int size) {
        head = new node(-1, size, 0, nullptr);
        cout << "Memory Initialized." << endl;
        print();
        return 1;
    }

    int myMalloc(int ID, int size) {
        pthread_mutex_lock(&lock);
        int r = 0;
        node* currentptr = head;
        node *prev=nullptr;

        while (currentptr != nullptr) {
            if (currentptr->id == -1 && currentptr->size > size) {
                node* newnode = new node(ID, size, currentptr->idx, currentptr);
                
                currentptr->size -= size;
                currentptr->idx += size;

                if (prev == nullptr) {
                    head = newnode; 
                }
                else{
                    prev->next = newnode;
                }

                r = newnode->idx;
                break;
            }
            else if (currentptr->id == -1 && currentptr->size == size) {
                node* newnode = new node(ID, size, currentptr->idx, currentptr->next);
                currentptr = newnode;
                r = newnode->idx;
                delete currentptr;
                break;
            }
            else {
            	prev = currentptr;
                currentptr = currentptr->next;
            }
        }

        if (currentptr == nullptr) {
            r = -1;
            cout << "Cannot allocate, requested size " << size << " for thread " << ID << " is bigger than remaining size." << endl;
        }
        else {
            cout << "Allocated for thread " << ID << endl;
        }
        print();
        pthread_mutex_unlock(&lock);
        return r;
    }

    int myFree(int ID, int index) {
        pthread_mutex_lock(&lock);
        int r = 0;
        node* currentptr = head;
	node* prev = nullptr;
        while (currentptr != nullptr) {
            if (currentptr->id == ID && currentptr->idx == index) {
                currentptr->id = -1;

                if (prev != nullptr && prev->id != -1 && currentptr->next != nullptr && currentptr->next->id == -1) {
                    currentptr->size += currentptr->next->size;  
                    node *temp = currentptr->next;
                    temp = currentptr->next->next;
                    delete temp;
                    
                    
                }
                else if (prev != nullptr && prev->id == -1 && currentptr->next != nullptr && currentptr->next->id != -1) {
                    prev->size += currentptr->size;
                    delete currentptr;
                    
                }
                else if (prev != nullptr && prev->id == -1 && currentptr->next != nullptr && currentptr->next->id == -1) {
                    prev->size += (currentptr->size + currentptr->next->size);
                    prev->next = currentptr->next->next;
                    delete currentptr->next;
                    delete currentptr;
                }
                r = 1;
                break;
            }
            else {
            	prev = currentptr;
                currentptr = currentptr->next;
            }
        }

        if (r == 0) {
            r = -1;
            cout << "Could not free for thread " << ID << endl;
        }
        else {
            cout << "Freed for thread " << ID << endl;
        }
        print();
        pthread_mutex_unlock(&lock);
        return r;
    }

    void print(){	
		node *printed = head;
		while(printed != NULL) {
			cout << "[" << printed->id << "]["  << printed->size << "][" << printed->idx << "]";
			
			printed = printed->next;
			cout << "----";				
		}
		cout << endl;
		
	}

private:
    node* head;
};

#endif

