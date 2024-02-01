#		$Id: Makefile,v 1.3 2005/07/30 21:11:02 root Exp root $

LOCALBASE?=	/usr/local

PROG=		gpioflicker
MAN=		gpioflicker.8
SRCS=		gpioflicker.c
CFLAGS+=	-Wall
LDADD+=		-lutil

MANDIR=		${LOCALBASE}/man/cat
BINDIR=		${LOCALBASE}/sbin

.include <bsd.prog.mk>
