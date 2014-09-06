#ifndef SRL_STACK_H_
#define SRL_STACK_H_

#include <stdlib.h>

#include "assert.h"
#include "srl_inline.h"
#include "srl_common.h"

#ifndef NDEBUG
#define DEBUG_ASSERT_STACK_SANE(stack) STMT_START {                           \
    assert((stack) != NULL);                                                  \
    assert((stack)->begin != NULL);                                           \
    assert((stack)->end != NULL);                                             \
    assert((stack)->begin < (stack)->end);                                    \
    assert((stack)->ptr == NULL ||                                            \
           ((stack)->ptr >= (stack)->begin && (stack)->ptr <= (stack)->end)); \
} STMT_END
#else
#define DEBUG_ASSERT_STACK_SANE(stack)
#endif

#define DEBUG_ASSERT_STACK_PTR(stack, ptr) assert((ptr) >= (stack)->begin && (ptr) <= (stack)->end)
#define DEBUG_ASSERT_STACK_VALUE(ptr) assert(*(ptr) >= 0)
#define DEBUG_ASSERT_STACK_PTR_VALUE(stack) DEBUG_ASSERT_STACK_VALUE((stack)->ptr)

//#define SRL_STACK_TRACE(msg, args...) warn(msg, args)
#define SRL_STACK_TRACE(msg, args...)

#define SRL_STACK_SIZE(stack)  (((stack)->end - (stack)->begin) + 1)
#define SRL_STACK_SPACE(stack) (((stack)->ptr - (stack)->begin) + 1)
#define SRL_STACK_POS(stack)   ((stack)->ptr ? (stack)->ptr - (stack)->begin : -1)

typedef struct {
    int64_t *begin, *end, *ptr;
} srl_stack_t;

/* Allocate new arrfer (but not the stack struct */
SRL_STATIC_INLINE int
srl_stack_init(srl_stack_t * stack, size_t size)
{
    assert(size > 0);
    assert(stack != NULL);

    stack->begin = NULL;
    Newx(stack->begin, size, int64_t);
    if (expect_false(stack->begin == NULL))
        return 1;

    stack->end = stack->begin + size - 1;
    stack->ptr = NULL;

    assert(SRL_STACK_SIZE(stack) == (int) size);
    return 0;
}

/* Free stack arrfer (not not the stack struct */
SRL_STATIC_INLINE void
srl_stack_destroy(pTHX_ srl_stack_t *stack)
{
    if (stack == NULL) return;
    Safefree(stack->begin);
}

SRL_STATIC_INLINE void
srl_stack_clear(pTHX_ srl_stack_t *stack)
{
    DEBUG_ASSERT_STACK_SANE(stack);
    stack->ptr = NULL;
}

#define srl_stack_ptr(stack) ((stack)->ptr)
#define srl_stack_empty(stack) ((stack)->ptr == NULL)
#define srl_stack_incr_value(stack, ptr, val) STMT_START { \
    DEBUG_ASSERT_STACK_SANE(stack);                        \
    DEBUG_ASSERT_STACK_PTR((stack), (ptr));                \
    *(ptr) += (val);                                       \
    DEBUG_ASSERT_STACK_VALUE(ptr);                         \
} STMT_END

SRL_STATIC_INLINE int64_t
srl_stack_peek_nocheck(pTHX_ srl_stack_t *stack)
{
    DEBUG_ASSERT_STACK_SANE(stack);
    DEBUG_ASSERT_STACK_PTR_VALUE(stack);
    return *stack->ptr;
}

SRL_STATIC_INLINE int64_t
srl_stack_peek(pTHX_ srl_stack_t *stack)
{
    DEBUG_ASSERT_STACK_SANE(stack);
    if (expect_false(srl_stack_empty(stack)))
        croak("srl_stack_peek on empty stack");

    return srl_stack_peek_nocheck(stack);
}

SRL_STATIC_INLINE void
srl_stack_push(pTHX_ srl_stack_t *stack, int64_t cnt)
{
    DEBUG_ASSERT_STACK_SANE(stack);
    if (expect_false(stack->ptr && stack->ptr >= stack->end)) {
        ptrdiff_t pos = SRL_STACK_POS(stack);
        size_t new_size = SRL_STACK_SIZE(stack) * 2;
        assert(new_size <= 1024 * 1024); // make some sanity

        Renew(stack->begin, new_size, int64_t);
        if (stack->begin == NULL)
            croak("Out of memory");

        stack->end = stack->begin + new_size - 1;
        stack->ptr = stack->begin + pos;
        DEBUG_ASSERT_STACK_SANE(stack);

        SRL_STACK_TRACE("grew stack to size %zu", new_size);
    }

    if (srl_stack_empty(stack)) {
        stack->ptr = stack->begin;
    } else {
        stack->ptr++;
    }

    *stack->ptr = cnt;

    DEBUG_ASSERT_STACK_SANE(stack);
    DEBUG_ASSERT_STACK_PTR_VALUE(stack);
    SRL_STACK_TRACE("pushed %d on stack, current idx %d",
                    (int) cnt, (int) SRL_STACK_POS(stack));
}

SRL_STATIC_INLINE void
srl_stack_pop(pTHX_ srl_stack_t *stack)
{
    DEBUG_ASSERT_STACK_SANE(stack);
    if (expect_false(srl_stack_empty(stack)))
        croak("Pop empty stack");

    if (stack->ptr == stack->begin) {
        stack->ptr = NULL;
    } else {
        stack->ptr--;
    }

    DEBUG_ASSERT_STACK_SANE(stack);
    SRL_STACK_TRACE("poped stack, current idx %d",
                    (int) SRL_STACK_POS(stack));
}

SRL_STATIC_INLINE int
__compare_int64_t(const void *a, const void *b)
{
    return (*(int64_t*) b - *(int64_t*) a);
}

SRL_STATIC_INLINE void
srl_stack_rsort(pTHX_ srl_stack_t *stack)
{
    DEBUG_ASSERT_STACK_SANE(stack);
    if (srl_stack_empty(stack)) return;
    qsort((void *) stack->begin, SRL_STACK_SPACE(stack), sizeof(int64_t), __compare_int64_t);
}

SRL_STATIC_INLINE void
srl_stack_dedupe(pTHX_ srl_stack_t *stack)
{
    DEBUG_ASSERT_STACK_SANE(stack);
    if (srl_stack_empty(stack)) return;

    int64_t *i = stack->begin;
    int64_t *j = stack->begin;
    for (; i <= stack->ptr; i++) {
        if (*j != *i) *++j = *i;
    }

    stack->ptr = j;
    DEBUG_ASSERT_STACK_SANE(stack);
}
#endif
