/**
 * Author: Qixiang Chao
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

typedef struct list_node {
    struct list_node *prev;
    struct list_node *next;
} list_node;


//typedef struct {
//    size_t size;
//    char allocated;
//};block_header;
//
//typedef struct {
//    size_t size;
//    int filler;
//};block_footer;


//typedef struct page_header {
//    struct page_header *prev;
//    struct page_header *next;
//    size_t page_size;
//} page_header;
typedef int block_header;
typedef int foot_header;

#define GET(p) (*(int *)(p))
#define PUT(p, val) (*(int *)(p) = (val))
#define PACK(size, alloc) ((size) | (alloc))
#define OVERHEAD (sizeof(block_header) + sizeof(foot_header))
#define HDPR(bp) ((char *)(bp) - sizeof(block_header)) //Header pointer
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define NEXT_BLKP(bp) ((char*)(bp)+GET_SIZE(HDPR(bp)))
#define PREV_BLKP(bp) ((char*)(bp)-GET_SIZE((char*)(bp)-OVERHEAD))
#define FTRP(bp) ((char*)(bp)+GET_SIZE(HDPR(bp))-OVERHEAD)
#define MAX(x, y) ((x) > (y)? (x) : (y))
#define NEXT_NODE(node) ((list_node *)(node))->next
#define PREV_NODE(node) ((list_node *)(node))->prev
//#define GET_PAGE_SIZE(ps) ((page_header *)(ps))->page_size
#define PRE_OCCUPIED (2 * OVERHEAD)


void *free_list_ptr;

void *extend(size_t newSize);

void set_allocated(void *bp, size_t size);

void *coalesce(void *bp);

void add_to_free_list(void *bp);

void remove_from_free_list(void *bp);

void *page_set_up(void *bp, size_t new_page_size);
//
//void *find_avail(size_t size);
//
//void *find_avail_recursion(size_t size, void *start_ptr);

//void blablabla(void *bp) {
//    printf("\n**************************\n");
//
//    printf("\n     The prev node: %p \n", PREV_NODE(bp));
//    printf("\n     The curr node: %p \n", bp);
//    printf("\n     The Next node: %p \n", NEXT_NODE(bp));
//
//    printf("\n**************************\n");
//
//}


void *page_set_up(void *bp, size_t new_page_size) {
    int this_size = (int) new_page_size;
    this_size = this_size - PRE_OCCUPIED;

    //  printf("bp address is 1 %p\n", bp);
    bp = (char *) bp + (OVERHEAD / 2);
    PUT(bp, PACK(8, 1));
    //  printf("bp address is 2 %p\n", bp);
    bp = (char *) bp + (OVERHEAD / 2);
    PUT(bp, PACK(8, 1));
    //  printf("bp address is 3 %p\n", bp);
    bp = (char *) bp + (OVERHEAD / 2);
    PUT(bp, PACK(this_size, 0));
    //  printf("bp address is 4 %p\n", bp);
    bp = (char *) bp + (OVERHEAD / 2);
    PUT(FTRP(bp), PACK(this_size, 0));
    PUT((FTRP(bp) + (OVERHEAD / 2)), PACK(0, 1));
    return bp;
}


/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
//    //Initial page size
//    size_t page_size = PAGE_ALIGN(mem_pagesize());
//
//    //The initial pointer, at the start of the page
//    first_bp = mem_map(page_size);
//    //First page pointer and current page pointer is the same location with first payload pointer
//    first_pp = current_page_pointer = first_bp;
//    free_list_ptr = first_bp + (3 * OVERHEAD);
//
//    //First page pointer has NULL previous page and NULL next page
//    NEXT_PAGE(first_pp) = NULL;
//    PREV_PAGE(first_pp) = NULL;
//
//    //Current page pointer does not as well
//    NEXT_PAGE(current_page_pointer) = NULL;
//    PREV_PAGE(current_page_pointer) = NULL;
//
//    SET_PAGE_SIZE(current_page_pointer, page_size);
//    SET_PAGE_SIZE(first_pp, page_size);
//
//    first_bp += PAGE_HEADER_SIZE + (OVERHEAD / 2);
//
//    //Setting up prologue header
//    PUT(HDPR(first_bp), PACK(16, 1));
//    //Setting up prologue footer
//    PUT(FTRP(first_bp), PACK(16, 1));
//
//    //Since we have an empty block at start, a prologue header and a prologue footer and a terminator,
//    //Thus available size should be total size minus 4 * Alignment(16)
//    current_avail_size = page_size - (3 * OVERHEAD);
//
//    //Global variable, every page has the same available page size
//    cons_avail_page_size = current_avail_size;
//
//    //This is the payload pointer for allocation
//    void *bp = first_bp + OVERHEAD;
//
//
//    PREV_NODE(free_list_ptr) = NULL;
//    NEXT_NODE(free_list_ptr) = NULL;
//
//
//    //Payload header has the size same with available size and allocated is 0
//    PUT(HDPR(bp), PACK(current_avail_size, 0));
//    //Payload footer
//    PUT(FTRP(bp), PACK(current_avail_size, 0));
//
//    //Terminator
//    PUT(HDPR(NEXT_BLKP(bp)), PACK(0, 1));
//
//    current_avail = bp;
//    //  printf("hi");
////    current_avail_size = 0;
//
//    return 0;

    free_list_ptr = NULL;
    return 0;
}

/*
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size) {
    if (size == 0) return NULL;
    int needed_size = MAX(size, sizeof(list_node));
    int new_size = ALIGN(needed_size + OVERHEAD);
    // printf("right now in the malloc, free list is %p\n", free_list_ptr);
    void *bp = free_list_ptr;
    while (bp) {
        if (!GET_ALLOC(HDPR(bp)) && GET_SIZE(HDPR(bp)) >= new_size) {
            set_allocated(bp, new_size);
            return bp;
        }

        bp = NEXT_NODE(bp);
    }
//    if((bp = find_avail(new_size)) != NULL)
//    {
//        set_allocated(bp, new_size);
//    }
    bp = extend(new_size);
    set_allocated(bp, new_size);
    return bp;

//
//    if (size == 0) return NULL;
//    int need_size = MAX(size, sizeof(list_node));
//
//    //Size should be aligned and add header and footer size
//    int new_size = ALIGN(need_size + OVERHEAD);
//    void *p = find_avail(new_size);
//    // printf("%d\n", new_size);
//
//    //printf("%d\n", GET_ALLOC(HDPR(NEXT_BLKP(p))));
//
//
//    if (p != NULL) {
//        set_allocated(p, new_size);
//        // printf("HHHH");
//        return p;
//    }
////    if ((p = find_avail_best_fit(new_size)) != NULL) {
////        set_allocated(p, new_size);
////        return p;
////    }
//
//    extend(new_size);
//    set_allocated(current_avail, new_size);
//    return p;
//
////    if (current_avail_size < newsize) {
////        current_avail_size = PAGE_ALIGN(newsize);
////        current_avail = mem_map(current_avail_size);
////        if (current_avail == NULL)
////            return NULL;
////    }
////
////    p = current_avail;
////    current_avail += newsize;
////    current_avail_size -= newsize;

}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp) {

//    size_t size;
//    if (ptr == NULL) return;
//
//    //Getting size of block via header
//    size = GET_SIZE(HDPR(ptr));
//
//    //Setting unallocated
//    PUT(HDPR(ptr), PACK(size, 0));
//    PUT(FTRP(ptr), PACK(size, 0));
//
//    //Merge if needed
//    coalesce(ptr);
//
//    if (GET_SIZE(HDPR(PREV_BLKP(ptr))) == OVERHEAD) {
//        current_page_pointer =
//                ptr -
//                (PAGE_HEADER_SIZE + OVERHEAD + (OVERHEAD / 2)); // HERE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//        //Double Linked List of pages
//        //If the page is all freed, the current page should be removed which is using "mem_unmap"
//        if (GET_SIZE(HDPR(ptr)) == (GET_PAGE_SIZE(current_page_pointer) - (3 * OVERHEAD))) {
//
//            //Setting a temp pointer point to the same with the current_page_pointer
//            void *temp_ptr = current_page_pointer;
//
//            size_t temp_size = GET_PAGE_SIZE(current_page_pointer);
//
//            //Case 1: If the page that is needed to free has both previous page and next page
//            if (NEXT_PAGE(current_page_pointer) != NULL && PREV_BLKP(current_page_pointer) != NULL) {
//
//                //current_page_pointer->next_page->prev_page = current_page_pointer->prev_page
//                PREV_PAGE(NEXT_PAGE(current_page_pointer)) = PREV_PAGE(current_page_pointer);
//
//                //current_page_pointer->prev_page->next_page = current_page_pointer->next_page
//                NEXT_PAGE(PREV_PAGE(current_page_pointer)) = NEXT_PAGE(current_page_pointer);
//
//                current_page_pointer = NEXT_PAGE(current_page_pointer);
//
//                mem_unmap(temp_ptr, temp_size);
//
//                //Case 2: If the page that is needed to free is the very top page
//            } else if (NEXT_PAGE(current_page_pointer) != NULL && PREV_PAGE(current_page_pointer) == NULL) {
//
//                //Setting current page pointer and first page pointer to the next page
//                current_page_pointer = first_pp = NEXT_PAGE(current_page_pointer);
//                //Setting first payload pointer to the next page as well
//                first_bp = first_pp + (2 * OVERHEAD);
//                PREV_PAGE(NEXT_PAGE(current_page_pointer)) = NULL;
//                // mem_unmap(temp_ptr, cons_avail_page_size + (4 * ALIGNMENT));
//                mem_unmap(temp_ptr, temp_size);
//                //Case 3: If the page is at the most bottom
//            } else if (NEXT_PAGE(current_page_pointer) == NULL && PREV_PAGE(current_page_pointer) != NULL) {
//
//                //Setting current page pointer goes to previous page
//                current_page_pointer = PREV_PAGE(current_page_pointer);
//                NEXT_PAGE(PREV_PAGE(current_page_pointer)) = NULL;
//                //  mem_unmap(temp_ptr, cons_avail_page_size + (4 * ALIGNMENT));
//                mem_unmap(temp_ptr, temp_size);
//            }
//        }
//    }
//    return;


    //printf("Im in free!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

    int this_alloc = GET_ALLOC(HDPR(bp));
    if (this_alloc == 1) {
        size_t size = GET_SIZE(HDPR(bp));
        PUT(HDPR(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        // printf("hit!@@@@@@@@@@@@@@@@@@@@@@@\n");
        bp = coalesce(bp);
        //  printf("in free, bp is at %p\n", bp);
        void *first_pp = HDPR(bp) - (OVERHEAD / 2) - OVERHEAD;
        void *prologue_header = HDPR(bp) - OVERHEAD;
        void *prologue_footer = HDPR(bp) - (OVERHEAD / 2);
        void *terminator = FTRP(bp) + (OVERHEAD / 2);

//        printf("This is in free, first page pointer is %p, prologue header %p, prologue footer %p, terminor %p",
//               first_pp, prologue_header, prologue_footer, terminator);

//        printf("This is in free, first page pointer is %d, prologue header %d, prologue footer %d, terminor %d",
//               first_pp, prologue_header, prologue_footer, terminator);
        if (GET_SIZE(prologue_footer) == OVERHEAD && GET_ALLOC(prologue_footer) &&
            GET_SIZE(prologue_header) == OVERHEAD && GET_ALLOC(prologue_header) && GET_ALLOC(terminator) &&
            !GET_SIZE(terminator)) {

            //      printf("in the if ---------free\n");
            remove_from_free_list(bp);
            mem_unmap(first_pp, GET_SIZE(HDPR(bp)) + PRE_OCCUPIED);

        }
    }
}

// void *find_avail(size_t size) {
////    void *bp;
////    for (bp = free_list_ptr; GET_ALLOC(HDPR(bp)) == 0; bp = NEXT_NODE(bp)) {
////
////        if (size <= GET_SIZE(HDPR(bp))) {
////            return bp;
////        }
////
////    }
////    return NULL;
//      return find_avail_recursion(size, free_list_ptr);
//}
//
////Simply first-fit algorithm
// void *find_avail_recursion(size_t size, void *start_ptr) {
//    if (!start_ptr) return NULL;
//    if (GET_ALLOC(HDPR(free_list_ptr)) == 0 && size <= GET_SIZE(HDPR(start_ptr))) return start_ptr;
//    return find_avail_recursion(size, NEXT_NODE(start_ptr));
//}


void set_allocated(void *bp, size_t size) {
    size_t full_size = GET_SIZE(HDPR(bp));
    size_t extra_size = full_size - size;
    if (extra_size > ALIGN(1 + OVERHEAD)) {
        PUT(HDPR(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        remove_from_free_list(bp);
        bp = NEXT_BLKP(bp);
        PUT(HDPR(bp), PACK(extra_size, 0));
        PUT(FTRP(bp), PACK(extra_size, 0));
        add_to_free_list(bp);
        //coalesce(bp);
    } else {
        PUT(HDPR(bp), PACK(full_size, 1));
        PUT(FTRP(bp), PACK(full_size, 1));
        remove_from_free_list(bp);
    }
}

void *coalesce(void *bp) {

    size_t size = GET_SIZE(HDPR(bp));

    void *terminator = FTRP(bp) + (OVERHEAD / 2);
    void *prologue = HDPR(bp) - (OVERHEAD / 2);
    int next_alloc, prev_alloc;
    if (GET_ALLOC(prologue) && GET_SIZE(prologue) == OVERHEAD) prev_alloc = 1;//Previous has header
    else prev_alloc = GET_ALLOC(HDPR(PREV_BLKP(bp)));


    if (GET_ALLOC(terminator) && !GET_SIZE(terminator)) next_alloc = 1; //Next is terminator
    else next_alloc = GET_ALLOC(HDPR(NEXT_BLKP(bp)));


    if (prev_alloc && next_alloc) {
        //    printf("Case 1 in \n");
        add_to_free_list(bp);
        return bp;
    }
        /**Case 2**/
    else if (prev_alloc && !next_alloc) {
        //    printf("Case 2 in \n");
        remove_from_free_list(NEXT_BLKP(bp));
        size += GET_SIZE(HDPR(NEXT_BLKP(bp)));

        PUT(HDPR(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        //  printf("STopped here!!\n");
        add_to_free_list(bp);
    }

        /**Case 3**/
    else if (!prev_alloc && next_alloc) {
        //  printf("Case 3 in \n");
        size += GET_SIZE(HDPR(PREV_BLKP(bp)));
        PUT(HDPR(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);

    }
        /**Case 4**/
    else if (!prev_alloc && !next_alloc) {
        //  printf("Case 4 in \n");
        remove_from_free_list(NEXT_BLKP(bp));
        size += (GET_SIZE(HDPR(PREV_BLKP(bp)))) + (GET_SIZE(HDPR(NEXT_BLKP(bp))));
        PUT(HDPR(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);

    }

    return bp;
}

void *extend(size_t size) {

//    int aligned_size = PAGE_ALIGN(size);
//    void *bp = mem_map(aligned_size);
//    NEXT_PAGE(current_page_pointer) = bp;
//    PREV_PAGE(bp) = current_page_pointer;
//    NEXT_PAGE(bp) = NULL;
//    current_page_pointer = bp;
//    SET_PAGE_SIZE(current_page_pointer, aligned_size);
//
//    bp += PAGE_HEADER_SIZE + (OVERHEAD / 2);
//    PUT(HDPR(bp), PACK(16, 1));
//    PUT(FTRP(bp), PACK(16, 1));
//    current_avail_size = aligned_size - (3 * OVERHEAD);
//    bp += OVERHEAD;
//    PUT(HDPR(bp), PACK(current_avail_size, 0));
//    PUT(FTRP(bp), PACK(current_avail_size, 0));
//    PUT(HDPR(NEXT_BLKP(bp)), PACK(0, 1));
//    current_avail = bp;
//    add_to_free_list(current_avail);
//
//

    //   printf("New Page !!!!!!!!!!!!!!!!!!!!!!! \n");
    size_t fit_size = MAX(mem_pagesize(), size);
    size_t needed_size = PAGE_ALIGN(fit_size * 16);
    void *bp = mem_map(needed_size);
    // printf("Im here stupoid!!!!!!\n");

    PUT(bp, PACK(0, 0));
    bp = page_set_up(bp, needed_size);
    //printf("bp is located at %p in extend!!!!!!\n", bp);
    add_to_free_list(bp);
    return bp;

}

void add_to_free_list(void *bp) {

    //printf("This %p\n", free_list_ptr);


    // printf("add free list called!!!!!!!!!!!!@@@@!!!!!!!!!!! \n");

    if (free_list_ptr == NULL) {
        // printf("free list is null in add to free list\n");
        //   blablabla(free_list_ptr);
        free_list_ptr = bp;

        PREV_NODE(free_list_ptr) = NULL;
        NEXT_NODE(free_list_ptr) = NULL;
    } else {

        // printf("free list is not null in add to free list\!!!!!!!!!!!!!!!!!!\n");
        //   blablabla(bp);
        NEXT_NODE(bp) = free_list_ptr;
        PREV_NODE(bp) = NULL;
        PREV_NODE(free_list_ptr) = bp;
        free_list_ptr = bp;
    }
}


void remove_from_free_list(void *bp) {

    if (bp == free_list_ptr) {
        if (PREV_NODE(bp)) free_list_ptr = PREV_NODE(bp);
        else if (NEXT_NODE(bp)) free_list_ptr = NEXT_NODE(bp);
        else free_list_ptr = NULL;
    }
    if (NEXT_NODE(bp) != NULL) PREV_NODE(NEXT_NODE(bp)) = PREV_NODE(bp);

    if (PREV_NODE(bp) != NULL) NEXT_NODE(PREV_NODE(bp)) = NEXT_NODE(bp);

}