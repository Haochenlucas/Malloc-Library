/*
 * mm-naive.c - The least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by allocating a
 * new page as needed.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* always use 16-byte alignment */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (8 * (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1)))

//8
typedef size_t block_header;

//8
typedef size_t block_footer;

#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))

#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_SIZE(p) (GET(p) & ~0xF)

#define PACK(size, alloc) ((size) | (alloc))

#define OVERHEAD (sizeof(block_header) + sizeof(block_footer))
#define HDRP(bp) ((char *) (bp) - sizeof(block_header))
#define FTRP(bp) ((char *) (bp) + GET_SIZE(HDRP(bp)) - OVERHEAD)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *) (bp) - OVERHEAD))

typedef struct list_node{
  struct list_node * prev;
  struct list_node * next;
}list_node;

#define GET_NEXT(p) ((list_node *) (p)) -> next
#define GET_PREV(p) ((list_node *) (p)) -> prev

void *current_avail = NULL;
int current_avail_size = 0;

void *freeList_start = NULL;
void *freeList_end = NULL;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  current_avail_size = PAGE_ALIGN(3 * ALIGNMENT);
  current_avail = mem_map(current_avail_size);

  if (current_avail == NULL){
    return -1;
  }

  // Keep the alignment
  PUT(current_avail, PACK(0, 1));
  current_avail += 8;

  // Set the starter
  PUT(current_avail, PACK(ALIGNMENT, 1));
  current_avail += 8;
  PUT(current_avail, PACK(ALIGNMENT, 1));
  current_avail += 8;

  // Remainning size minus two pair of overhead
  current_avail_size = current_avail_size - 2 * ALIGNMENT;

  // Set the remaining page
  current_avail += 8;
  PUT(HDRP(current_avail),PACK(current_avail_size, 0));
  PUT(FTRP(current_avail), PACK(current_avail_size, 0));

  // Set the ternimator
  PUT(HDRP(NEXT_BLKP(current_avail)),PACK(0, 1));

  freeList_start = current_avail;
  freeList_end = current_avail;
  GET_PREV(current_avail) = NULL;
  GET_NEXT(current_avail) = NULL;

  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  size_t newsize = ALIGN(size + ALIGNMENT);
  void *p = freeList_end;
  
  void *best_bp = NULL;

  if (freeList_start != NULL){
    while (1){
      if (!GET_ALLOC(HDRP(p)) && GET_SIZE(HDRP(p)) >= newsize) {

        set_allocated(p, newsize);
        return p;

        // if (!best_bp || GET_SIZE(HDRP(p)) < GET_SIZE(HDRP(best_bp))){
        //   best_bp = p;
        // }
       }
       if (p == freeList_start){
        break;
       }
       p = GET_PREV(p);
     }
   }

  // if(best_bp){
  //   set_allocated (best_bp, newsize);
  //   return best_bp;
  // }

  void *extend_p = extend(newsize);
  if (extend_p == NULL){
    return NULL;
  }

  set_allocated(extend_p, newsize);

  return extend_p;
}

void *extend(size_t new_size)
{
  size_t chunk_size = PAGE_ALIGN(new_size + ALIGNMENT * 2);
  void *bp = mem_map(chunk_size);

  if (bp == NULL){
    return NULL;
  }

  // Keep the alignment
  PUT(bp, PACK(0, 1));
  bp += 8;

  // Set the starter
  PUT(bp, PACK(ALIGNMENT, 1));
  bp += 8;
  PUT(bp, PACK(ALIGNMENT, 1));
  bp += 8;

  // Remainning size minus two pair of overhead
  chunk_size = chunk_size - (2 * ALIGNMENT);

  // Set the remaining page
  bp += 8;
  PUT(HDRP(bp),PACK(chunk_size, 0));
  PUT(FTRP(bp), PACK(chunk_size, 0));

  // Set the ternimator
  PUT(HDRP(NEXT_BLKP(bp)),PACK(0, 1));

  if (freeList_start == NULL && freeList_end == NULL)
  {
    freeList_start = bp;
    freeList_end = bp;
    GET_PREV(bp) = NULL;
    GET_NEXT(bp) = NULL;
  }
  else{
    GET_NEXT(freeList_end) = bp;
    GET_PREV(bp) = freeList_end;
    GET_NEXT(bp) = NULL;
    freeList_end = bp;
  }

  return bp; 
}

void set_allocated(void *bp, size_t size)
{
  size_t total_size = GET_SIZE(HDRP(bp));
  size_t extra_size = GET_SIZE(HDRP(bp)) - size;

  if (extra_size >= ALIGN(1 + OVERHEAD)){

    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));

    PUT(HDRP(NEXT_BLKP(bp)), PACK(extra_size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(extra_size, 0));
    replace_list_node(bp, size);
  }
  else
  {
    PUT(HDRP(bp), PACK(total_size, 1));
    PUT(FTRP(bp), PACK(total_size, 1));

    remove_list_node(bp);
  }
}

void replace_list_node(void *p, size_t distance)
{
  if (p == freeList_start && p == freeList_end){
    freeList_start += distance;
    freeList_end += distance;
    return;
  }

  // NOTE
  if (p == freeList_start){
    GET_PREV(GET_NEXT(freeList_start)) = (char *)GET_PREV(GET_NEXT(freeList_start)) + distance;
    void *next = GET_NEXT(freeList_start);
    freeList_start = (char *)freeList_start + distance;
    GET_NEXT(freeList_start) = next;
    GET_PREV(freeList_start) = NULL;
    return;
  }

  // NOTE
  if (p == freeList_end){
    GET_NEXT(GET_PREV(freeList_end)) = (char *)GET_NEXT(GET_PREV(freeList_end)) + distance;
    void *prev = GET_PREV(freeList_end);
    freeList_end = (char *)freeList_end + distance;
    GET_PREV(freeList_end) = prev;
    GET_NEXT(freeList_end) = NULL;
    return;
  }

  if (p != freeList_start && p != freeList_end){
    void *temp = p;

    void *prev = GET_PREV(p);
    GET_NEXT(prev) = (char *)p + distance;

    void *next = GET_NEXT(p);
    GET_PREV(next) = (char *)p + distance;

    p = (char *)p + distance;
    GET_PREV(p) = prev;
    GET_NEXT(p) = next;

    return;
  }
    return;
}

void remove_list_node(void *p)
{
  if (p == freeList_start && p == freeList_end){
    freeList_start = NULL;
    freeList_end = NULL;
    return;
  }

  if (p == freeList_start){
    freeList_start = GET_NEXT(freeList_start);
    GET_PREV(freeList_start) = NULL;
    return;
  }

  if (p == freeList_end){
    freeList_end = GET_PREV(freeList_end);
    GET_NEXT(freeList_end) = NULL;
    return;
  }

  if (p != freeList_start && p != freeList_end){
    void *prev = GET_PREV(p);
    void *next = GET_NEXT(p);
    GET_NEXT(prev) = next;
    GET_PREV(next) = prev;
  }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  size_t freesize = GET_SIZE(HDRP(ptr));

  PUT(HDRP(ptr), PACK(freesize , 0));
  PUT(FTRP(ptr), PACK(freesize , 0));

  void *checker = coalesce(ptr);

  if (!GET_ALLOC(checker) && GET_SIZE(HDRP(PREV_BLKP(checker))) == 16 && GET_SIZE(HDRP(NEXT_BLKP(checker))) == 0){
    if (checker == freeList_start && checker == freeList_end){
      freeList_start = NULL;
      freeList_end = NULL;
    }
    else if (checker == freeList_start){
      void *second = GET_NEXT(freeList_start);
      freeList_start = second;
      GET_PREV(second) = NULL;
    }
    else if (checker == freeList_end){
      void *last_second = GET_PREV(freeList_end);
      freeList_end = last_second;
      GET_NEXT(last_second) = NULL;
    }
    else{
      void *prev = GET_PREV(checker);
      void *next = GET_NEXT(checker);

      GET_NEXT(prev) = next;
      GET_PREV(next) = prev;
    }

    mem_unmap((checker - 2 * OVERHEAD), GET_SIZE(HDRP(checker)) + 32);
  }
}

void *coalesce(void *bp){
  size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  // no merge
  if (prev_alloc && next_alloc){
    GET_NEXT(freeList_end) = bp;
    GET_PREV(bp) = freeList_end;
    GET_NEXT(bp) = NULL;
    freeList_end = bp;
  }

  // merge with the next block
  else if (prev_alloc && !next_alloc){
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    void *nextBLK = NEXT_BLKP(bp);

    if ((nextBLK == freeList_start) && (nextBLK == freeList_end))
    {
      freeList_start = bp;
      freeList_end = bp;
      GET_PREV(bp) = NULL;
      GET_NEXT(bp) = NULL;
    }
    else
    {
      if (nextBLK == freeList_start){
        GET_NEXT(bp) = GET_NEXT(freeList_start);
        GET_PREV(GET_NEXT(freeList_start)) = bp;
        GET_PREV(bp) = NULL;
        freeList_start = bp;
      }
      else if (nextBLK == freeList_end){
        GET_PREV(bp) = GET_PREV(freeList_end);
        GET_NEXT(GET_PREV(freeList_end)) = bp;
        GET_NEXT(bp) = NULL;
        freeList_end = bp;


      }
      else{
        GET_PREV(bp) = GET_PREV(nextBLK);
        GET_NEXT(bp) = GET_NEXT(nextBLK);
        GET_NEXT(GET_PREV(nextBLK)) = bp;
        GET_PREV(GET_NEXT(nextBLK)) = bp;
      }
    }

    PUT(HDRP(bp), PACK(size , 0));
    PUT(FTRP(bp), PACK(size , 0));
  }

  // merge with the previous block
  else if (!prev_alloc && next_alloc){
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    bp = PREV_BLKP(bp);
    PUT(HDRP(bp), PACK(size , 0));
    PUT(FTRP(bp), PACK(size , 0));
  }

  // merge with both blocks
  else{
    void *prevBLK = PREV_BLKP(bp);
    void *nextBLK = NEXT_BLKP(bp);
    size += (GET_SIZE(HDRP(prevBLK)) + GET_SIZE(HDRP(nextBLK)));
    bp = PREV_BLKP(bp);
    if((prevBLK == freeList_start && nextBLK == freeList_end) 
      || (prevBLK == freeList_end && nextBLK == freeList_start)){

      if (((prevBLK == freeList_start && nextBLK == freeList_end) && (GET_NEXT(prevBLK) == nextBLK))
        || ((prevBLK == freeList_end && nextBLK == freeList_start) && (GET_NEXT(nextBLK) == prevBLK))){

        freeList_start = prevBLK;
        freeList_end = prevBLK;
        GET_PREV(freeList_start) = NULL;
        GET_NEXT(freeList_end) = NULL;
      }
      else{
        // Previous block is the starter of the free list
        if (GET_PREV(prevBLK) == NULL){
          freeList_end = GET_PREV(nextBLK);
          GET_NEXT(freeList_end) = NULL;
        }
        // Previous block is the end of the free list
        else{
          freeList_end = GET_PREV(prevBLK);
          GET_NEXT(freeList_end) = NULL;

          freeList_start = prevBLK;
          GET_NEXT(freeList_start) = GET_NEXT(nextBLK);
          GET_PREV(GET_NEXT(nextBLK)) = freeList_start;
          GET_PREV(freeList_start) = NULL;
        } 
      }
    }

    else if (prevBLK == freeList_start || nextBLK == freeList_start){
      if (prevBLK == freeList_start){
        void *prev = GET_PREV(nextBLK);
        void *next = GET_NEXT(nextBLK);

        GET_NEXT(prev) = next;
        GET_PREV(next) = prev;
      }
      else{
        void *prev = GET_PREV(prevBLK);
        void *next = GET_NEXT(prevBLK);

        GET_NEXT(prev) = next;
        GET_PREV(next) = prev;

        GET_PREV(GET_NEXT(nextBLK)) = prevBLK;
        GET_NEXT(prevBLK) = GET_NEXT(nextBLK);
        GET_PREV(prevBLK) = NULL;
        freeList_start = prevBLK;
      }
    }

    else if (prevBLK == freeList_end || nextBLK == freeList_end){
      if (prevBLK == freeList_end){
        GET_NEXT(GET_PREV(nextBLK)) = GET_NEXT(nextBLK);
        GET_PREV(GET_NEXT(nextBLK)) = GET_PREV(nextBLK);
      }
      else{
        void *prev = GET_PREV(prevBLK);
        void *next = GET_NEXT(prevBLK);

        GET_NEXT(prev) = next;
        GET_PREV(next) = prev;

        GET_NEXT(prevBLK) = NULL;
        GET_PREV(prevBLK) = GET_PREV(nextBLK);
        GET_NEXT(GET_PREV(nextBLK)) = prevBLK;
        freeList_end = prevBLK;
      }
    }
    else{
      GET_NEXT(GET_PREV(nextBLK)) = GET_NEXT(nextBLK);
      GET_PREV(GET_NEXT(nextBLK)) = GET_PREV(nextBLK);
    }

    PUT(HDRP(bp), PACK(size , 0));
    PUT(FTRP(bp), PACK(size , 0));
  }
  return bp; 
}