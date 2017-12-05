/**
 * @file 
 * @brief Heap for storing candidate groups of variables for various algorithms.
 *
 * @author Antti Rasinen
 * @author Janne Toivola
 * @author Mikko Korpela
 * @copyright &copy; 2007,2012 Janne Toivola <br>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. <br>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. <br>
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __NIPHEAP_H__
#define __NIPHEAP_H__

#include "niplists.h"

#define NIP_HEAP_PARENT(i) ((i-1)/2) ///< index for parent of i
#define NIP_HEAP_LEFT(i)  (2*i+1)    ///< index for left child of i
#define NIP_HEAP_RIGHT(i) (2*(i+1))  ///< index for right child of i

/**
 * Structure for storing an item in a heap
 */
typedef struct {
  int content_size;  ///< in case content is an array (always at least 1)
  void* content;     ///< the pointer to the content (possibly array)
  int primary_key;   ///< key determining the order in the heap
  int secondary_key; ///< secondary key determining the order in case of ties
} nip_heap_item_struct;
typedef nip_heap_item_struct* nip_heap_item; ///< reference to a heap item

/**
 * Structure storing the heap items and related state
 */
typedef struct {
  int allocated_size;        ///< Size of the currently allocated table
  nip_heap_item* heap_items; ///< The array of elements
  int heap_size;             ///< Number of elements inserted to the heap
  int (*primary_key)(void* item, int size);   ///< computes primary key
  int (*secondary_key)(void* item, int size); ///< computes secondary key
  int heapified;    ///< Flag if someone has seen the trouble of heapifying
  nip_int_list updated_items; /**< List of indices of updated elements 
				 which potentially violate heap property */
} nip_heap_struct;
typedef nip_heap_struct* nip_heap; ///< reference to a heap


/**
 * Creates a new heap
 * @param initial_size Some guess for required size
 * @param primary function for evaluating primary key
 * @param secondary function for evaluating secondary key
 * @return reference to a new heap
 * @see nip_free_heap() */
nip_heap nip_new_heap(int initial_size,
		      int (*primary)(void* item, int size),
		      int (*secondary)(void* item, int size));


/**
 * Inserts a new element into the heap h. 
 * The heap property is not valid after this, so remember to heapify...
 * @param h The heap
 * @param content Pointer to the stored element
 * @param size Size of \p content */
int nip_heap_insert(nip_heap h, void* content, int size);


/**
 * Makes a linear search through all items in the heap by using 
 * the supplied comparison operation and reference content. 
 * Returns a heap index to be used for reference in the 
 * nip_set_heap_item function, or -1 if not found.
 * @param h The heap
 * @param comparison The comparison function
 * @param ref The content to search
 * @param refsize Size of content 
 * @return Index of the heap item, or -1 if not found */
int nip_search_heap_item(nip_heap h, 
			 int (*comparison)(void* i, int isize, 
					   void* r, int rsize), 
			 void* ref, int refsize);


/**
 * Retrieves the content stored to given index (random access to heap).
 * Returns the void pointer contained by the heap and size of the content 
 * (or 0 in case of error) via given pointer.
 * @param h The heap
 * @param index Flat index to the heap
 * @param size Pointer to which the returned content size is written
 * @return pointer to the found content */
void* nip_get_heap_item(nip_heap h, int index, int* size);


/**
 * Replaces the heap item referenced by index (given by nip_search_heap_item)
 * with the given content.
 * @param h The heap
 * @param index Flat index to the heap
 * @param content Stuff to be stored
 * @param size Size of \p content
 * @return error code, or 0 if successful */
int nip_set_heap_item(nip_heap h, int index, void* content, int size);


/**
 * Makes the heap obey the heap property after modifications to the root
 * @param h The heap */
void nip_build_min_heap(nip_heap h);


/**
 * Returns the least expensive item from the heap. The content pointer is 
 * returned and size of the content returned via int pointer. If the heap 
 * is empty, NULL pointer and 0 is returned.
 * @param h The heap
 * @param size Pointer where the content size is written
 * @return found "minimal" content or NULL */
void* nip_heap_extract_min(nip_heap h, int* size);


/**
 * Frees the memory allocated to the heap.
 * Does not free the contents, so don't free a heap unless it's empty
 * or you have pointers to the (dynamically allocated) content somewhere.
 * @param h The heap to free */
void nip_free_heap(nip_heap h);


/**
 * Tells how many elements the heap contains
 * @param h The heap
 * @return Number of elements inserted to the heap */
int nip_heap_size(nip_heap h);

#endif /* __HEAP_H__ */
