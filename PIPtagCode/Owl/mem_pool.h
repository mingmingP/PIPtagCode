#ifndef TPIP_MEM_POOL_H_
#define TPIP_MEM_POOL_H_

/*******************************************************************************
 * "Dynamic" memory pool allocation to make it easier to allocate memory for
 * data during sensing. The proper way to use this is to allocate memory for
 * sensed data, give all of the allocated memory to the radio for transmission,
 * and then free all of the memory after transmitting it and before the next
 * round of sensing.
 * TLDR:
 * 1. Call 'malloc(size)' for each new piece of sensed data.
 * 2. Send all of the data to the radio, starting at location 'getBase()' and
 * of length 'getAllSize()'.
 * 3. Call freeAll() after sending the data.
 * WARNING: Do not use more then MAX_MEM_POOL bytes.
 ******************************************************************************/

#include "../CC110x/definitions.h"
#define MAX_MEM_POOL 20
/*
 * Allocate the given number of bytes in the memory pool and return a pointer
 * to the address of the first byte.
 */
uint8_t* malloc(unsigned int size);

/*
 * Return the base address of all allocated memory.
 */
uint8_t* getBase();

/*
 * Return the total number of bytes allocated.
 */
int getAllSize();

/*
 * Returns true if the contents of the memory pool from the last time that
 * freeAll() was called differ from the data before freeAll was called.
 */
bool dataChanged();

/*
 * Deallocate all bytes.
 */
void freeAll();

#endif /* TPIP_MEM_POOL_H_ */
