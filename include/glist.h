/*
 Copyright (c) 2017 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

#ifndef GLIST_H_
#define GLIST_H_

#define GLIST_LINK(TYPE) \
    TYPE * prev; \
    TYPE * next;

#define GLIST_HEAD(NAME) glist_##NAME##_head
#define GLIST_TAIL(NAME) glist_##NAME##_tail

#define GLIST_BEGIN(NAME) GLIST_HEAD(NAME).next
#define GLIST_END(NAME) &GLIST_TAIL(NAME)

#define GLIST_ADD(NAME,ELEMENT) \
    GLIST_TAIL(NAME).prev->next = ELEMENT; \
    ELEMENT->prev = GLIST_TAIL(NAME).prev; \
    GLIST_TAIL(NAME).prev = ELEMENT; \
    ELEMENT->next = &GLIST_TAIL(NAME);

#define GLIST_REMOVE(NAME,ELEMENT) \
    ELEMENT->prev->next = ELEMENT->next; \
    ELEMENT->next->prev = ELEMENT->prev;

#define GLIST_CLEAN_ALL(NAME,CLEAN) \
    while (GLIST_BEGIN(NAME) != GLIST_END(NAME)) { \
        CLEAN(GLIST_BEGIN(NAME)); \
    }

#define GLIST_INST(TYPE,NAME,CLEAN) \
    static TYPE GLIST_HEAD(NAME); \
    static TYPE GLIST_TAIL(NAME); \
    void GLIST_##NAME##_constructor(void) __attribute__((constructor)); \
    void GLIST_##NAME##_constructor(void) { \
        GLIST_HEAD(NAME).next = &GLIST_TAIL(NAME); \
        GLIST_TAIL(NAME).prev = &GLIST_HEAD(NAME); \
    } \
    void GLIST_##NAME##_destructor(void) __attribute__((destructor)); \
    void GLIST_##NAME##_destructor(void) { \
        GLIST_CLEAN_ALL(NAME,CLEAN) \
    }

#endif /* GLIST_H_ */
