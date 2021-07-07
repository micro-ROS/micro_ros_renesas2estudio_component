
#include <microros_allocators.h>

#include <stdlib.h>
#include <string.h>

#ifdef MICROROS_FREERTOS_ALLOCATORS

#include "FreeRTOS.h"
#include "task.h"

void vPortFree(void * pv);
void *pvPortMalloc(size_t xWantedSize);
void *pvPortRealloc(void *pv, size_t xWantedSize);
void *pvPortCalloc(size_t num, size_t xWantedSize);

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE         ( ( size_t ) 8 )

/* Define the linked list structure.  This is used to link free blocks in order
 * of their memory address. */
typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK * pxNextFreeBlock; /*<< The next free block in the list. */
    size_t xBlockSize;                     /*<< The size of the free block. */
} BlockLink_t;

static const size_t xHeapStructSize = ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
static size_t  xBlockAllocatedBit = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 );

void *pvPortRealloc(void *pointer, size_t xWantedSize)
{
    vTaskSuspendAll();

    if (xWantedSize == 0)
    {
        vPortFree(pointer);
        return NULL;
    }

    void *ptr = pvPortMalloc(xWantedSize);
    if (ptr)
    {
        // Get size of pv
        uint8_t *puc = (uint8_t *) pointer;
        puc -= xHeapStructSize;

        BlockLink_t *pxLink = (void *) puc;

        // Copy and free *pointer memory
        if (pointer != NULL && (pxLink->xBlockSize & xBlockAllocatedBit) != 0)
        {
            size_t pv_size = pxLink->xBlockSize & ~xBlockAllocatedBit;
            memcpy(ptr, pointer, pv_size);
            vPortFree(pointer);
        }
    }

    ( void ) xTaskResumeAll();
    return ptr;
}

void *pvPortCalloc( size_t num, size_t xWantedSize )
{
    vTaskSuspendAll();

    void *pointer = pvPortMalloc(num * xWantedSize);

    if (pointer)
    {
        // Zero the memory
        memset(pointer, 0, num * xWantedSize);
    }

  (void) xTaskResumeAll();
  return pointer;
}

#endif

void * microros_allocate(size_t size, void * state){
  (void) state;
#ifdef MICROROS_FREERTOS_ALLOCATORS
  return pvPortMalloc(size);
#else
  void * ptr = malloc(size);
  return ptr;
#endif
}

void microros_deallocate(void * pointer, void * state){
  (void) state;
#ifdef MICROROS_FREERTOS_ALLOCATORS
  if (NULL != pointer){
    vPortFree(pointer);
  }
#else
  free(pointer);
#endif
}

void * microros_reallocate(void * pointer, size_t size, void * state){
  (void) state;
#ifdef MICROROS_FREERTOS_ALLOCATORS
  if (NULL == pointer){
    return pvPortMalloc(size);
  } else {
    return pvPortRealloc(pointer,size);
  }
#else
  void * ptr = realloc(pointer, size);
  return ptr;
#endif
}

void * microros_zero_allocate(size_t number_of_elements, size_t size_of_element, void * state){
  (void) state;
#ifdef MICROROS_FREERTOS_ALLOCATORS
  return pvPortCalloc(number_of_elements, size_of_element);
#else
  void * ptr = malloc(number_of_elements * size_of_element);
  memset(ptr, 0, number_of_elements * size_of_element);
  return ptr;
#endif
}
