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
    size_t page_size;
} page_header;

typedef size_t block_header; //Should be aligned to 16 bytes instead of keeping 8 bytes
typedef size_t foot_header;

#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))
#define PACK(size, alloc) ((size) | (alloc))
#define OVERHEAD (sizeof(block_header) + sizeof(foot_header))
#define HDPR(bp) ((char *)(bp) - (OVERHEAD / 2)) //Header pointer
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDPR(bp)))//Next block pointer
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char*)(bp) - OVERHEAD)) // Previous block pointer
#define FTRP(bp) ((char *)(bp)+GET_SIZE(HDPR(bp))-OVERHEAD)
#define MAX(x, y) ((x) > (y)? (x) : (y))
#define NEXT_PAGE(pp) ((page_header *)(pp))->next
#define PREV_PAGE(pp) ((page_header *)(pp))->prev
#define NEXT_NODE(node) ((list_node *)(node))->next
#define PREV_NODE(node) ((list_node *)(node))->prev
#define SET_NEXT_PTR(node, qp) (NEXT_NODE(node) = qp)
#define SET_PREV_PTR(node, qp) (PREV_NODE(node) = qp)
#define GET_PAGE_SIZE(ps) ((page_header *)(ps))->page_size
#define SET_PAGE_SIZE(ps, ns) (GET_PAGE_SIZE(ps) = ns)

//#define GET_SIZE(p) (block_header *)(p)->size
//#define GET_ALLOC(p) (block_header *)(p)->allocated
//#define SET_SIZE(p, ns) (GET_SIZE(p) = ns)
//#define SET_ALLOC(p, na) (GET_ALLOC(p) = na)



//Question1: Footer and header alignment
//Question2: Where should set the start of the free list pointer
//Question3: mem_unmap pointer and page issue
//Question4: Linking of double linked list
//Question5: If statement in "find_avail_recursion"
//Question6: Free list linking
//Question7: Footer filler setting



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

static void *find_avail_best_fit(size_t size);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    //Initial page size
    size_t page_size = PAGE_ALIGN(mem_pagesize());

    //The initial pointer, at the start of the page
    first_bp = mem_map(page_size);
    //First page pointer and current page pointer is the same location with first payload pointer
    first_pp = current_page_pointer = first_bp;

    PUT(first_bp, 0);
    SET_PAGE_SIZE(current_page_pointer, page_size);
    SET_PAGE_SIZE(first_pp, page_size);

    //First page pointer has NULL previous page and NULL next page
    NEXT_PAGE(first_pp) = NULL;
    PREV_PAGE(first_pp) = NULL;

    //Current page pointer does not as well
    NEXT_PAGE(current_page_pointer) = NULL;
    PREV_PAGE(current_page_pointer) = NULL;

    //Overhead equals 16
    first_bp += OVERHEAD;

    //Setting up prologue header
    PUT(HDPR(first_bp), PACK(16, 1));
    //Setting up prologue footer
    PUT(FTRP(first_bp), PACK(16, 1));

    //The first free list pointer
    //free_list_ptr = first_bp;

    //Since we have an empty block at start, a prologue header and a prologue footer and a terminator,
    //Thus available size should be total size minus 4 * Alignment(16)
    current_avail_size = page_size - (2 * OVERHEAD);

    //Global variable, every page has the same available page size
    cons_avail_page_size = current_avail_size;

    //This is the payload pointer for allocation
    void *bp = first_bp + OVERHEAD;
    free_list_ptr = bp;

    //Payload header has the size same with available size and allocated is 0
    PUT(HDPR(bp), PACK(current_avail_size, 0));
    //Payload footer
    PUT(FTRP(bp), PACK(current_avail_size, 0));

    //Terminator
    PUT(HDPR(NEXT_BLKP(bp)), PACK(0, 1));

    current_avail = bp;
    //  printf("hi");
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

    //Size should be aligned and add header and footer size
    int new_size = ALIGN(need_size + OVERHEAD);
    void *p;
    if ((p = find_avail(new_size)) != NULL) {

        set_allocated(p, new_size);
        return p;
    }
//    if ((p = find_avail_best_fit(new_size)) != NULL) {
//        set_allocated(p, new_size);
//        return p;
//    }

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

    //Getting size of block via header
    size = GET_SIZE(HDPR(ptr));

    //Setting unallocated
    PUT(HDPR(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    //Merge if needed
    coalesce(ptr);

    if (GET_SIZE(PREV_BLKP(ptr)) == OVERHEAD) {
        current_page_pointer = ptr - (2 * OVERHEAD); // HERE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        //Double Linked List of pages
        //If the page is all freed, the current page should be removed which is using "mem_unmap"
        if (GET_SIZE(HDPR(ptr)) == (GET_PAGE_SIZE(current_page_pointer) - (2 * OVERHEAD))) {
            //Setting current page pointer
            //  current_page_pointer = ptr - (2 * OVERHEAD);

            //Setting a temp pointer point to the same with the current_page_pointer
            void *temp_ptr = current_page_pointer;

            size_t temp_size = GET_PAGE_SIZE(current_page_pointer);

            //Case 1: If the page that is needed to free has both previous page and next page
            if (NEXT_PAGE(current_page_pointer) != NULL && PREV_BLKP(current_page_pointer) != NULL) {

                //current_page_pointer->next_page->prev_page = current_page_pointer->prev_page
                PREV_PAGE(NEXT_PAGE(current_page_pointer)) = PREV_PAGE(current_page_pointer);

                //current_page_pointer->prev_page->next_page = current_page_pointer->next_page
                NEXT_PAGE(PREV_PAGE(current_page_pointer)) = NEXT_PAGE(current_page_pointer);


                //Current page pointer goes to next page
                current_page_pointer = NEXT_PAGE(current_page_pointer);

                //un_map from temp ptr with length of available and 4 * alignment
                //      mem_unmap(temp_ptr, cons_avail_page_size + (4 * ALIGNMENT)); //Question !!!!!!!!!!!!!!!!!!

                mem_unmap(temp_ptr, temp_size);

                //Case 2: If the page that is needed to free is the very top page
            } else if (NEXT_PAGE(current_page_pointer) != NULL && PREV_PAGE(current_page_pointer) == NULL) {

                //Setting current page pointer and first page pointer to the next page
                current_page_pointer = first_pp = NEXT_PAGE(current_page_pointer);
                //Setting first payload pointer to the next page as well
                first_bp = first_pp + (2 * OVERHEAD);
                PREV_PAGE(NEXT_PAGE(current_page_pointer)) = NULL;
                // mem_unmap(temp_ptr, cons_avail_page_size + (4 * ALIGNMENT));
                mem_unmap(temp_ptr, temp_size);
                //Case 3: If the page is at the most bottom
            } else if (NEXT_PAGE(current_page_pointer) == NULL && PREV_PAGE(current_page_pointer) != NULL) {

                //Setting current page pointer goes to previous page
                current_page_pointer = PREV_PAGE(current_page_pointer);
                NEXT_PAGE(PREV_PAGE(current_page_pointer)) = NULL;
                //  mem_unmap(temp_ptr, cons_avail_page_size + (4 * ALIGNMENT));
                mem_unmap(temp_ptr, temp_size);
            }
        }
    }
    return;
}

static void *coalesce(void *ptr) {
    //Get size, next and previous allocated
    size_t next_alloc = GET_ALLOC(HDPR(NEXT_BLKP(ptr)));
    size_t prev_alloc = GET_ALLOC(HDPR(PREV_BLKP(ptr)));
    size_t size = GET_SIZE(HDPR(ptr));

    /**Case 1**/
    if (prev_alloc && next_alloc) {/*DO NOTHING*/}
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
    PUT(bp, 0);
    NEXT_PAGE(current_page_pointer) = bp;
    PREV_PAGE(bp) = current_page_pointer;
    NEXT_PAGE(bp) = NULL;
    current_page_pointer = bp;
    SET_PAGE_SIZE(current_page_pointer, aligned_size);

    bp += OVERHEAD;
    PUT(HDPR(bp), PACK(16, 1));
    PUT(FTRP(bp), PACK(16, 0));
    current_avail_size = aligned_size - (2 * OVERHEAD);
    bp += OVERHEAD;
    PUT(HDPR(bp), PACK(current_avail_size, 0));
    PUT(FTRP(bp), PACK(current_avail_size, 0));
    PUT(HDPR(NEXT_BLKP(bp)), PACK(0, 1));
    current_avail = bp;
    coalesce(current_avail);
}

static void *find_avail(size_t size) {
//    void *bp;
//    for (bp = free_list_ptr; GET_ALLOC(HDPR(bp)) == 0; bp = NEXT_NODE(bp)) {
//
//        if (size <= GET_SIZE(HDPR(bp))) {
//            return bp;
//        }
//
//    }
//    return NULL;
    return find_avail_recursion(size, free_list_ptr);
}

//Simply first-fit algorithm
static void *find_avail_recursion(size_t size, void *start_ptr) {
    if (!start_ptr) return NULL;
    if (GET_ALLOC(HDPR(free_list_ptr)) == 0 && size <= GET_SIZE(HDPR(start_ptr))) return start_ptr;
    return find_avail_recursion(size, NEXT_NODE(start_ptr));
}


static void *find_avail_best_fit(size_t size) {
    void *p, *best_bp = NULL;
    for (p = free_list_ptr; GET_ALLOC(HDPR(p)) == 0; p = NEXT_NODE(p)) {
        if (p) {
            if (size <= GET_SIZE(HDPR(p))) {
                if (!best_bp || (GET_SIZE(HDPR(p)) < GET_SIZE(HDPR(best_bp)))) {
                    best_bp = p;
                    // return best_bp;
                }
            }
        } else break;
    }
    return (best_bp == NULL) ? NULL : best_bp;
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

