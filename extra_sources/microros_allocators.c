
#include <unistd.h>

#undef MICROROS_FREERTOS_ALLOCATORS

void *pvPortMallocMicroROS( size_t xWantedSize );
void vPortFreeMicroROS( void *pv );
void *pvPortReallocMicroROS( void *pv, size_t xWantedSize );
size_t getBlockSize( void *pv );
void *pvPortCallocMicroROS( size_t num, size_t xWantedSize );

void * microros_allocate(size_t size, void * state){
  (void) state;
#ifdef MICROROS_FREERTOS_ALLOCATORS
  return pvPortMallocMicroROS(size);
#else 
  void * ptr = malloc(size);
  return ptr;
#endif
}

void microros_deallocate(void * pointer, void * state){
  (void) state;
#ifdef MICROROS_FREERTOS_ALLOCATORS
  if (NULL != pointer){
    vPortFreeMicroROS(pointer);
  }
#else
  free(pointer);
#endif
}

void * microros_reallocate(void * pointer, size_t size, void * state){
  (void) state;
#ifdef MICROROS_FREERTOS_ALLOCATORS
  if (NULL == pointer){
    return pvPortMallocMicroROS(size);
  } else {
    return pvPortReallocMicroROS(pointer,size);
  }
#else
  void * ptr = realloc(pointer, size);
  return ptr;
#endif
}

void * microros_zero_allocate(size_t number_of_elements, size_t size_of_element, void * state){
  (void) state;
#ifdef MICROROS_FREERTOS_ALLOCATORS
  return pvPortCallocMicroROS(number_of_elements, size_of_element);
#else
  void * ptr = malloc(number_of_elements * size_of_element);
  memset(ptr, 0, number_of_elements * size_of_element);
  return ptr;
#endif
}
