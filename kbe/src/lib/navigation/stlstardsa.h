/* 

A* Algorithm Implementation using STL is
Copyright (C)2025-2030 Mason

Permission is given by the author to freely redistribute and 
include this code in any program as long as this credit is 
given where due.
 
  COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS, 
  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, 
  INCLUDING, WITHOUT LIMITATION, WARRANTIES THAT THE COVERED CODE 
  IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
  OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND 
  PERFORMANCE OF THE COVERED CODE IS WITH YOU. SHOULD ANY COVERED 
  CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT THE INITIAL 
  DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY 
  NECESSARY SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF 
  WARRANTY CONSTITUTES AN ESSENTIAL PART OF THIS LICENSE. NO USE 
  OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
  THIS DISCLAIMER.
 
  Use at your own risk!



  DynamicSizeAllocator class
  Copyright 2025 Mason

*/

#ifndef STLSTARDSA_H
#define STLSTARDSA_H
#include <iostream> 
#include <cstring> 
 
template <class USER_TYPE> 
class DynamicSizeAllocator { 
public: 
    // Constants 
    enum { 
        DSA_DEFAULT_SIZE = 100 
    }; 
 
    // This class enables us to transparently manage the extra data 
    // needed to enable the user class to form part of the double-linked 
    // list class 
    struct DSA_ELEMENT { 
        USER_TYPE UserType; 
        DSA_ELEMENT *pPrev; 
        DSA_ELEMENT *pNext; 
    }; 
 
public: // methods 
    DynamicSizeAllocator(unsigned int InitialElements = DSA_DEFAULT_SIZE) :
        m_pFirstFree(NULL), 
        m_pFirstUsed(NULL), 
        m_MaxElements(0) { 
        allocateMoreMemory(InitialElements); 
    } 
 
    ~DynamicSizeAllocator() { 
        // Free up all the allocated memory 
        while (m_pMemoryChunks) { 
            MemoryChunk* temp = m_pMemoryChunks; 
            m_pMemoryChunks = m_pMemoryChunks->next; 
            delete[] (char*)temp->memory; 
            delete temp; 
        } 
    } 
 
    // Allocate a new USER_TYPE and return a pointer to it 
    USER_TYPE *alloc() { 
        DSA_ELEMENT *pNewNode = NULL; 
 
        if (!m_pFirstFree) { 
            // If there are no free nodes, allocate more memory 
            allocateMoreMemory(m_MaxElements ? m_MaxElements : DSA_DEFAULT_SIZE); 
            if (!m_pFirstFree) { 
                return NULL; 
            } 
        } 
 
        pNewNode = m_pFirstFree; 
        m_pFirstFree = pNewNode->pNext; 
 
        // if the new node points to another free node then 
        // change that nodes prev free pointer... 
        if (pNewNode->pNext) { 
            pNewNode->pNext->pPrev = NULL; 
        } 
 
        // node is now on the used list 
 
        pNewNode->pPrev = NULL; // the allocated node is always first in the list 
 
        if (m_pFirstUsed == NULL) { 
            pNewNode->pNext = NULL; // no other nodes 
        } 
        else { 
            m_pFirstUsed->pPrev = pNewNode; // insert this at the head of the used list 
            pNewNode->pNext = m_pFirstUsed; 
        } 
 
        m_pFirstUsed = pNewNode; 
 
        return reinterpret_cast<USER_TYPE*>(pNewNode); 
    } 
 
    // Free the given user type 
    // For efficiency I don't check whether the user_data is a valid 
    // pointer that was allocated. I may add some debug only checking 
    // (To add the debug check you'd need to make sure the pointer is in 
    // the m_pMemory area and is pointing at the start of a node) 
    void free(USER_TYPE *user_data) { 
        DSA_ELEMENT *pNode = reinterpret_cast<DSA_ELEMENT*>(user_data); 
 
        // manage used list, remove this node from it 
        if (pNode->pPrev) { 
            pNode->pPrev->pNext = pNode->pNext; 
        } 
        else { 
            // this handles the case that we delete the first node in the used list 
            m_pFirstUsed = pNode->pNext; 
        } 
 
        if (pNode->pNext) { 
            pNode->pNext->pPrev = pNode->pPrev; 
        } 
 
        // add to free list 
        if (m_pFirstFree == NULL) { 
            // free list was empty 
            m_pFirstFree = pNode; 
            pNode->pPrev = NULL; 
            pNode->pNext = NULL; 
        } 
        else { 
            // Add this node at the start of the free list 
            m_pFirstFree->pPrev = pNode; 
            pNode->pNext = m_pFirstFree; 
            m_pFirstFree = pNode; 
        } 
    } 
 
    // For debugging this displays both lists (using the prev/next list pointers) 
    void Debug() { 
        printf("free list "); 
 
        DSA_ELEMENT *p = m_pFirstFree; 
        while (p) { 
            printf("%x!%x ", (unsigned int)p->pPrev, (unsigned int)p->pNext); 
            p = p->pNext; 
        } 
        printf("\n"); 
 
        printf("used list "); 
 
        p = m_pFirstUsed; 
        while (p) { 
            printf("%x!%x ", (unsigned int)p->pPrev, (unsigned int)p->pNext); 
            p = p->pNext; 
        } 
        printf("\n"); 
    } 
 
    // Iterators 
 
    USER_TYPE *GetFirst() { 
        return reinterpret_cast<USER_TYPE *>(m_pFirstUsed); 
    } 
 
    USER_TYPE *GetNext(USER_TYPE *node) { 
        return reinterpret_cast<USER_TYPE *>( 
            (reinterpret_cast<DSA_ELEMENT *>(node))->pNext 
            ); 
    } 
 
private: 
    struct MemoryChunk { 
        DSA_ELEMENT* memory; 
        MemoryChunk* next; 
        MemoryChunk(DSA_ELEMENT* mem) : memory(mem), next(NULL) {} 
    }; 
 
    void allocateMoreMemory(unsigned int numElements) { 
        // Allocate enough memory for the specified number of elements 
        char *pMem = new char[numElements * sizeof(DSA_ELEMENT)]; 
        DSA_ELEMENT *newMemory = (DSA_ELEMENT *)pMem; 
 
        // Add the new memory chunk to the list 
        MemoryChunk* newChunk = new MemoryChunk(newMemory); 
        newChunk->next = m_pMemoryChunks; 
        m_pMemoryChunks = newChunk; 
 
        // Set the free list first pointer if it's the first allocation 
 /*       if (!m_pFirstFree) { 
            m_pFirstFree = newMemory; 
        } */
 
        // Clear the memory 
        memset(newMemory, 0, sizeof(DSA_ELEMENT) * numElements); 
 
        // Point at first element 
        DSA_ELEMENT *pElement = newMemory; 
 
        // Set the double linked free list 
        for (unsigned int i = 0; i < numElements; i++) { 
            pElement->pPrev = pElement - 1; 
            pElement->pNext = pElement + 1; 
 
            pElement++; 
        } 
 
        // first element should have a null prev 
        newMemory->pPrev = NULL; 
        // last element should have a null next 
        (pElement - 1)->pNext = NULL; 
 
        // Connect the new free list to the existing one 
        if (m_pFirstFree) { 
            // 找到主链表的最后一个节点
            DSA_ELEMENT* lastFree = m_pFirstFree;
            while (lastFree->pNext) {
                lastFree = lastFree->pNext;
            }
            // 将新内存块链接到主链表尾部 
            lastFree->pNext = newMemory;
            newMemory->pPrev = lastFree;
        }
        else {
            m_pFirstFree = newMemory; 
        }
 
        m_MaxElements += numElements; 
    } 
 
private: 
    DSA_ELEMENT *m_pFirstFree; 
    DSA_ELEMENT *m_pFirstUsed; 
    unsigned int m_MaxElements; 
    MemoryChunk* m_pMemoryChunks = NULL; 
}; 
 
 

#endif // defined STLSTARDSA_H
