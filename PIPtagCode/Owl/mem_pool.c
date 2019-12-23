#include "mem_pool.h"

//Memory pool storage
//Keep two pools so we can quickly check for a change in sensed data
uint8_t buff_a[MAX_MEM_POOL];
uint8_t buff_b[MAX_MEM_POOL];
//Start and current location both start at buff_a
uint8_t* start_ptr = buff_a;
uint8_t* cur_ptr = buff_a;

//Data needed for the dataChanged function.
uint8_t last_length = 0;


/*
 * Allocate the given number of bytes in the memory pool and return a pointer
 * to the address of the first byte.
 */
uint8_t* malloc(unsigned int size) {
	uint8_t* begin = cur_ptr;
	//Zero the memory as we increment cur_ptr.
	while (cur_ptr - begin < size) {
		*cur_ptr = 0;
		++cur_ptr;
	}
	return begin;
}

/*
 * Return the base address of all allocated memory.
 */
uint8_t* getBase() {
	return start_ptr;
}

/*
 * Return the total number of bytes allocated.
 */
int getAllSize() {
	return (cur_ptr - start_ptr);
}

/*
 * Returns true if the contents of the memory pool from the last time that
 * freeAll() was called differ from the data before freeAll was called.
 */
bool dataChanged() {
	if (last_length != getAllSize()) {
		return true;
	}
	bool changed = false;
	{
		int i = 0;
		for (; i < last_length; ++i) {
			changed |= (buff_a[i] ^ buff_b[i]);
		}
	}
	return changed;
}

/*
 * Deallocate all bytes.
 */
void freeAll() {
	//Store the length of the last round of data storage to speed up
	//calls to the dataChanged function.
	last_length = getAllSize();
	//Alternate between the two buffers for the dataChange function
	if (start_ptr == buff_a) {
		start_ptr = buff_b;
	}
	else {
		start_ptr = buff_a;
	}
	//Finally reset the current pointer to the new buffer.
	cur_ptr = start_ptr;
}

