#include <boot/isr.h>

extern const void * _stack_start;

typedef void(*ISR)(void);

void hard_fault_handler();
void mem_fault_handler();
void bus_fault_handler();
void usage_fault_handler();

/* STM32H725 exposes interrupt numbers up to OCTOSPI2_IRQn=150, so the vector
 * table must be at least 16 core entries + 151 external entries. */
#define INITIALISATION_VECTOR_SIZE 0xA7

ISR InitialisationVector[INITIALISATION_VECTOR_SIZE]
  __attribute__((section(".isr_vector_table")))
  __attribute__((used))
  = {
  [0] = (ISR)&_stack_start,
  [1] = start,
  [2] = abort,
  [3] = hard_fault_handler,
  [4] = mem_fault_handler,
  [5] = bus_fault_handler,
  [6] = usage_fault_handler,
  [11] = abort,
  [12] = abort,
  [14] = abort,
  [15] = isr_systick
};
