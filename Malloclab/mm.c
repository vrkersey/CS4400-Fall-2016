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
//#include <tclDecls.h>

#include "mm.h"
#include "memlib.h"
/* always use 16-byte alignment */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))

//typedef struct {
//    size_t size;
//    char allocated;
//};block_header;
//
//typedef struct {
//    size_t size;
//    int filler;
//};block_footer;

typedef struct list_node {

    struct list_node *prev;
    struct list_node *next;
} list_node;

typedef struct page_header {
    struct page_header *prev;
    struct page_header *next;
} page_header;

typedef size_t block_header;
typedef size_t foot_header;

#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))
#define PACK(size, alloc) ((size) | (alloc))
#define OVERHEAD (sizeof(block_header) + sizeof(foot_header))
#define HDPR(bp) ((char *)(bp) - OVERHEAD) //Header pointer
#define GET_SIZE(p) (GET(p) & ~0xF)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDPR(bp)))//Next block pointer
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE((char*)(bp) - OVERHEAD)) // Previous block pointer
#define FTRP(bp) ((char*)(bp) + GET_SIZE((char*)(bp) - OVERHEAD)) // Footer pointer
#define MAX(x, y) ((x) > (y)? (x) : (y))
#define NEXT_PAGE(pp) ((page_header *)(pp))->next
#define PREV_PAGE(pp) ((page_header *)(pp))->prev
#define NEXT_NODE(node) ((list_node *)(node))->next
#define PREV_NODE(node) ((list_node *)(node))->prev
#define SET_NEXT_PTR(node, qp) (NEXT_NODE(node) = qp)
#define SET_PREV_PTR(node, qp) (PREV_NODE(node) = qp)


void *first_pp = NULL; // Page Pointer
void *first_bp = NULL;
void *current_avail = NULL;
int current_avail_size = 0;
int cons_avail_page_size = 0;
void *free_list_ptr = NULL;
void *current_page_pointer = NULL;


static void extend(size_t newSize);

static void set_allocated(void *bp, size_t size);

static void *coalesce(void *bp);

static void add_to_free_list(void *bp);

static void remove_from_free_list(void *bp);

static void *find_avail_recursion(size_t size, void *start_ptr);

static void *find_avail(size_t size);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    size_t page_size = PAGE_ALIGN(mem_pagesize());

    first_bp = mem_map(page_size);
    first_pp = current_page_pointer = first_bp;
    NEXT_PAGE(first_pp) = NULL;
    PREV_PAGE(first_pp) = NULL;
    NEXT_PAGE(current_page_pointer) = NULL;
    PREV_PAGE(current_page_pointer) = NULL;

    first_bp += OVERHEAD;
    PUT(HDPR(first_bp), PACK(2, 1));
    PUT(FTRP(first_bp), PACK(2, 0));
    free_list_ptr = first_bp;
    current_avail_size = page_size - (4 * ALIGNMENT);
    cons_avail_page_size = current_avail_size;
    void *bp = first_bp + OVERHEAD;
    PUT(HDPR(bp), PACK(current_avail_size, 0));
    PUT(FTRP(bp), current_avail_size);
    PUT(HDPR(NEXT_BLKP(bp)), PACK(0, 1));

    current_avail = bp;
//    current_avail_size = 0;

    return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size) {
    if (size == 0) return NULL;
    int need_size = MAX(size, sizeof(list_node));
    int new_size = ALIGN(need_size + OVERHEAD);
    void *p;
    if ((p = find_avail(new_size)) != NULL) {
        set_allocated(p, new_size);
        return p;
    }
    extend(new_size);
    set_allocated(p, new_size);
    return p;

//    if (current_avail_size < newsize) {
//        current_avail_size = PAGE_ALIGN(newsize);
//        current_avail = mem_map(current_avail_size);
//        if (current_avail == NULL)
//            return NULL;
//    }
//
//    p = current_avail;
//    current_avail += newsize;
//    current_avail_size -= newsize;

}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    size_t size;
    if (ptr == NULL) return;
    size = GET_SIZE(HDPR(ptr));
    PUT(HDPR(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
    if (GET_SIZE(HDPR(ptr)) == cons_avail_page_size) {
        current_page_pointer = ptr - (2 * OVERHEAD);
        if (NEXT_PAGE(current_page_pointer) != NULL && PREV_BLKP(current_page_pointer) != NULL) {
            PREV_PAGE(NEXT_PAGE(current_page_pointer)) = PREV_PAGE(current_page_pointer);
            NEXT_PAGE(PREV_PAGE(current_page_pointer)) = NEXT_PAGE(current_page_pointer);
            void *temp_ptr = current_page_pointer;
            current_page_pointer = NEXT_PAGE(current_page_pointer);
            mem_unmap(temp_ptr, cons_avail_page_size + (4 * ALIGNMENT)); //Question !!!!!!!!!!!!!!!!!!

        } else if (NEXT_PAGE(current_page_pointer) != NULL && PREV_PAGE(current_page_pointer) == NULL) {
            void *temp_ptr = current_page_pointer;
            current_page_pointer = first_pp = NEXT_PAGE(current_page_pointer);
            first_bp = first_pp + (2 * OVERHEAD);
            PREV_PAGE(NEXT_PAGE(current_page_pointer)) = NULL;
            mem_unmap(temp_ptr, cons_avail_page_size + (4 * ALIGNMENT));

        } else if (NEXT_PAGE(current_page_pointer) == NULL && PREV_PAGE(current_page_pointer) != NULL) {
            void *temp_ptr = current_page_pointer;
            current_page_pointer = PREV_PAGE(current_page_pointer);
            NEXT_PAGE(PREV_PAGE(current_page_pointer)) = NULL;
            mem_unmap(temp_ptr, cons_avail_page_size + (4 * ALIGNMENT));
        }
    }
    return;
}

static void *coalesce(void *ptr) {
    size_t next_alloc = GET_ALLOC(HDPR(NEXT_BLKP(ptr)));
    size_t prev_alloc = GET_ALLOC(HDPR(PREV_BLKP(ptr)));
    size_t size = GET_SIZE(HDPR(ptr));

    /**Case 1**/
    if (prev_alloc && next_alloc) add_to_free_list(ptr);

        /**Case 2**/
    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDPR(NEXT_BLKP(ptr)));
        remove_from_free_list(NEXT_BLKP(ptr));
        PUT(HDPR(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));
    }

        /**Case 3**/
    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDPR(PREV_BLKP(ptr)));
        ptr = PREV_BLKP(ptr);
        remove_from_free_list(ptr);
        PUT(HDPR(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));
    }

        /**Case 4**/
    else if (!prev_alloc && !next_alloc) {
        size += (GET_SIZE(HDPR(PREV_BLKP(ptr)))) + (GET_SIZE(HDPR(NEXT_BLKP(ptr))));
        remove_from_free_list(PREV_BLKP(ptr));
        remove_from_free_list(NEXT_BLKP(ptr));
        ptr = PREV_BLKP(ptr);
        PUT(HDPR(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));
    }
    add_to_free_list(ptr);
    return ptr;
}

static void extend(size_t size) {
//    int need_size = MAX(size, sizeof(list_node));
//    int newsize = ALIGN(need_size + OVERHEAD);
    int aligned_size = PAGE_ALIGN(size);
    void *bp = mem_map(aligned_size);

    NEXT_PAGE(current_page_pointer) = bp;
    PREV_PAGE(bp) = current_page_pointer;
    NEXT_PAGE(bp) = NULL;
    current_page_pointer = bp;

    bp += OVERHEAD;
    PUT(HDPR(bp), PACK(2, 1));
    PUT(FTRP(bp), PACK(2, 0));
    current_avail_size = aligned_size - (4 * ALIGNMENT);
    bp += OVERHEAD;
    PUT(HDPR(bp), PACK(current_avail_size, 0));
    PUT(FTRP(bp), current_avail_size);
    PUT(HDPR(NEXT_BLKP(bp)), PACK(0, 1));
    current_avail = bp;
}

static void *find_avail(size_t size) {
    return find_avail_recursion(size, free_list_ptr);
}

static void *find_avail_recursion(size_t size, void *start_ptr) {
    if (!start_ptr) return NULL;
    if (GET_ALLOC(HDPR(free_list_ptr)) == 0 && size <= GET_SIZE(HDPR(start_ptr))) return start_ptr;
    return find_avail_recursion(size, NEXT_NODE(start_ptr));
}

static void set_allocated(void *bp, size_t size) {
    size_t full_size = GET_SIZE(HDPR(bp));
    size_t extra_size = full_size - size;
    if (extra_size > ALIGN(1 + OVERHEAD)) {
        PUT(HDPR(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        remove_from_free_list(bp);
        bp = NEXT_BLKP(bp);
        PUT(HDPR(bp), PACK(extra_size, 0));
        PUT(FTRP(bp), PACK(extra_size, 0));
        coalesce(bp);
    } else {
        PUT(HDPR(bp), PACK(full_size, 1));
        PUT(FTRP(bp), PACK(full_size, 1));
        remove_from_free_list(bp);
    }
}

static void add_to_free_list(void *bp) {
    SET_NEXT_PTR(bp, free_list_ptr);
    SET_PREV_PTR(free_list_ptr, bp);
    SET_PREV_PTR(bp, NULL);
    free_list_ptr = bp;
}

static void remove_from_free_list(void *bp) {
    if (PREV_NODE(bp))
        SET_NEXT_PTR(PREV_NODE(bp), NEXT_NODE(bp));
    else
        free_list_ptr = NEXT_NODE(bp);

    SET_PREV_PTR(NEXT_NODE(bp), PREV_NODE(bp));

}


