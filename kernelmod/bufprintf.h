/* SPDX-License-Identifier: GPL-2.0-only */
/*
* Copyright (c) 2018 Cable Television Laboratories, Inc. ("CableLabs")
*                    and others.  All rights reserved.
*
* Created by Jon Dennis (j.dennis@cablelabs.com)
*/

#ifndef BUFPRINTF_H_INCLUDED
#define BUFPRINTF_H_INCLUDED


struct bufprintf_buf {
  unsigned   len;
  char      *ptr;
  char       buf[ 20 * 1024 ];
};

#define bufprintf_init(BUFPTR) {(BUFPTR)->len = 0; (BUFPTR)->ptr = (BUFPTR)->buf; (BUFPTR)->buf[0] = '\0';}
#define bufprintf(BUFPTR, ...) {BUFPTR->ptr += snprintf(BUFPTR->ptr, (sizeof(BUFPTR->buf) - (BUFPTR->ptr - BUFPTR->buf)), __VA_ARGS__ ); if ((BUFPTR->ptr - BUFPTR->buf) >= sizeof(BUFPTR->buf)) BUFPTR->ptr = BUFPTR->buf + sizeof(BUFPTR->buf); BUFPTR->len = BUFPTR->ptr - BUFPTR->buf;}


#endif /* #ifndef BUFPRINTF_H_INCLUDED */
