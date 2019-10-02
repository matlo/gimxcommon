/*
 Copyright (c) 2017 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

#ifndef GLIST_H_
#define GLIST_H_

#define GLIST_LINK(TYPE) \
    TYPE * prev, * next

#define GLIST_HEAD(NAME) glist_##NAME.head
#define GLIST_TAIL(NAME) glist_##NAME.tail

#define GLIST_IS_EMPTY(NAME) (GLIST_HEAD(NAME).next == &GLIST_TAIL(NAME))

#define GLIST_BEGIN(NAME) GLIST_HEAD(NAME).next
#define GLIST_END(NAME) &GLIST_TAIL(NAME)

#define GLIST_ADD(NAME, ELEMENT) \
    do { \
        GLIST_TAIL(NAME).prev->next = ELEMENT; \
        ELEMENT->prev = GLIST_TAIL(NAME).prev; \
        GLIST_TAIL(NAME).prev = ELEMENT; \
        ELEMENT->next = &GLIST_TAIL(NAME); \
    } while (0)

#define GLIST_REMOVE(NAME, ELEMENT) \
    do { \
        ELEMENT->prev->next = ELEMENT->next; \
        ELEMENT->next->prev = ELEMENT->prev; \
    } while (0)

#define GLIST_CLEAN_ALL(NAME, CLEAN) \
    while (GLIST_BEGIN(NAME) != GLIST_END(NAME)) { \
        CLEAN(GLIST_BEGIN(NAME)); \
    }

#define GLIST_INST(TYPE, NAME) \
    struct { \
        TYPE head; \
        TYPE tail; \
    } glist_##NAME = { \
        .head = { .next = &GLIST_TAIL(NAME) }, \
        .tail = { .prev = &GLIST_HEAD(NAME) }, \
    }

#define GLIST_DESTRUCTOR(NAME, CLEAN) \
    void GLIST_##NAME##_destructor(void) __attribute__((destructor)); \
    void GLIST_##NAME##_destructor(void) { \
        GLIST_CLEAN_ALL(NAME, CLEAN) \
    }

#endif /* GLIST_H_ */
