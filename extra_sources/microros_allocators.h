
#ifndef MICROROS_ALLOCATORS__H
#define MICROROS_ALLOCATORS__H

#include <stddef.h>
#include "hal_data.h"

#if (BSP_CFG_RTOS == 2)
// FreeRTOS
#define MICROROS_FREERTOS_ALLOCATORS
#endif

void * microros_allocate(size_t size, void * state);
void microros_deallocate(void * pointer, void * state);
void * microros_reallocate(void * pointer, size_t size, void * state);
void * microros_zero_allocate(size_t number_of_elements, size_t size_of_element, void * state);

#endif  // MICROROS_ALLOCATORS__H
