/**
  * @file traps.h
  * @author phuong.do
  * @date 2017-06-13
  * @brief Managing all the information about traps in the system
  */
#ifndef __TRAPS_H__
#define __TRAPS_H__

#include "private.h"

/**
  * @brief Contain list of int3 breakpoints set at same position
  */
typedef struct int3break_t {
    GSList *traplist;
} int3break_t;

/**
  * @brief Init trap manager
  * @param vm Pointer to vmhdlr_t struct
  * @return Return pointer to trapmngr_t if init success, or else return NULL
  */
trapmngr_t *traps_init(vmhdlr_t *vm);

/**
  * @brief Destroy trap manager, releases all resources
  * @param Pointer to trapmngr_t
  */
void traps_destroy(trapmngr_t *traps);

/**
  * @brief Add a remapped page to remapped hashtable
  * @param traps Pointer to trapmng_t
  * @param original Original gfn
  * @param remapped Remapped gfn
  */
void traps_add_remapped(trapmngr_t *traps, uint64_t original, uint64_t remapped);

/**
  * @brief Find a remapped page of a frame
  * @param traps Pointer to trapmngr
  * @param origin Original frame number
  * @return Return address of remap of this frame, or return 0 if not exist
  */
uint64_t traps_find_remapped(trapmngr_t *traps, uint64_t original);

int3break_t *traps_find_breakpoint(trapmngr_t *traps, uint64_t pa);

void traps_add_breakpoint(trapmngr_t *traps, uint64_t *pa, int3break_t *wrapper);
#endif