#-------------------------------------------------------------------------
#
# Makefile for src/bin (client programs)
#
# Portions Copyright (c) 1996-2023, PostgreSQL Global Development Group
# Portions Copyright (c) 1994, Regents of the University of California
#
# src/bin/Makefile
#
#-------------------------------------------------------------------------

subdir = src/bin
top_builddir = ../..
include $(top_builddir)/src/Makefile.global

.NOTPARALLEL:
# incl. https://www.postgresql.org/docs/current/reference-client.html
# excl. https://www.postgresql.org/docs/current/reference-server.html
SUBDIRS = \
	pg_amcheck \
	pg_basebackup \
	pg_config \
	pg_dump \
	pg_verifybackup \
	pgbench \
	psql \
	scripts

ifeq ($(PORTNAME), win32)
else
ALWAYS_SUBDIRS += pgevent
endif

$(recurse)
$(recurse_always)
