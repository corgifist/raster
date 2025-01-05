// Borrowed from https://github.com/mtrebi/memory-allocators/tree/master
// Single-header free list allocator, ideal for managing caches

#ifndef FREELISTALLOCATOR_H
#define FREELISTALLOCATOR_H

#ifndef SINGLYLINKEDLIST_H
#define SINGLYLINKEDLIST_H

#include <cstddef>

template <class T>
class SinglyLinkedList {
public:
    struct Node {
        T data;
        Node * next;
    };
    
    Node * head;
    
public:
    SinglyLinkedList();

    void insert(Node * previousNode, Node * newNode);
    void remove(Node * previousNode, Node * deleteNode);
};

template <class T>
SinglyLinkedList<T>::SinglyLinkedList(){
    
}

template <class T>
void SinglyLinkedList<T>::insert(Node* previousNode, Node* newNode){
    if (previousNode == nullptr) {
        // Is the first node
        if (head != nullptr) {
            // The list has more elements
            newNode->next = head;           
        }else {
            newNode->next = nullptr;
        }
        head = newNode;
    } else {
        if (previousNode->next == nullptr){
            // Is the last node
            previousNode->next = newNode;
            newNode->next = nullptr;
        }else {
            // Is a middle node
            newNode->next = previousNode->next;
            previousNode->next = newNode;
        }
    }
}

template <class T>
void SinglyLinkedList<T>::remove(Node* previousNode, Node* deleteNode){
    if (previousNode == nullptr){
        // Is the first node
        if (deleteNode->next == nullptr){
            // List only has one element
            head = nullptr;            
        }else {
            // List has more elements
            head = deleteNode->next;
        }
    }else {
        previousNode->next = deleteNode->next;
    }
}

#endif /* SINGLYLINKEDLIST_H */

struct FreeListAllocator {
    enum PlacementPolicy {
        FIND_FIRST,
        FIND_BEST
    };

    struct FreeHeader {
        std::size_t blockSize;
    };
    struct AllocationHeader {
        std::size_t blockSize;
        char padding;
    };
    
    typedef SinglyLinkedList<FreeHeader>::Node Node;
    
    void* m_start_ptr;
    PlacementPolicy m_pPolicy;
    SinglyLinkedList<FreeHeader> m_freeList;

    FreeListAllocator(const std::size_t totalSize, const PlacementPolicy pPolicy);
    FreeListAllocator();

    ~FreeListAllocator();

    void* Allocate(const std::size_t size, const std::size_t alignment = 8);

    void Free(void* ptr);

    void Init();

    void Reset();

    std::size_t m_totalSize, m_used, m_peak;
    FreeListAllocator(FreeListAllocator &freeListAllocator);

    void Coalescence(Node* prevBlock, Node * freeBlock);

    void Find(const std::size_t size, const std::size_t alignment, std::size_t& padding, Node*& previousNode, Node*& foundNode);
    void FindBest(const std::size_t size, const std::size_t alignment, std::size_t& padding, Node*& previousNode, Node*& foundNode);
    void FindFirst(const std::size_t size, const std::size_t alignment, std::size_t& padding, Node*& previousNode, Node*& foundNode);
};

#endif /* FREELISTALLOCATOR_H */

#ifndef UTILS_H
#define UTILS_H

class Utils {
public:
	static const std::size_t CalculatePadding(const std::size_t baseAddress, const std::size_t alignment) {
		const std::size_t multiplier = (baseAddress / alignment) + 1;
		const std::size_t alignedAddress = multiplier * alignment;
		const std::size_t padding = alignedAddress - baseAddress;
		return padding;
	}

	static const std::size_t CalculatePaddingWithHeader(const std::size_t baseAddress, const std::size_t alignment, const std::size_t headerSize) {
		std::size_t padding = CalculatePadding(baseAddress, alignment);
		std::size_t neededSpace = headerSize;

		if (padding < neededSpace){
			// Header does not fit - Calculate next aligned address that header fits
			neededSpace -= padding;

			// How many alignments I need to fit the header        
        	if(neededSpace % alignment > 0){
		        padding += alignment * (1+(neededSpace / alignment));
        	}else {
		        padding += alignment * (neededSpace / alignment);
        	}
		}

		return padding;
	}
};

#endif /* UTILS_H */
#include <stdlib.h>     /* malloc, free */
#include <cassert>   /* assert		*/
#include <limits>  /* limits_max */
#include <algorithm>    // std::max

#ifdef _DEBUG
#include <iostream>
#endif