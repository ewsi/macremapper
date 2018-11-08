#!/bin/bash


rm -Rf INSTALL Makefile.in aclocal.m4 ar-lib compile config.guess config.sub configure depcomp install-sh ltmain.sh m4/ missing mrmctl/Makefile.in mrmfilterparser/Makefile.in
autoreconf --install
rm -Rf autom4te.cache
