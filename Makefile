all:	tasconfig
	mysqldump -d tasmota > tasmota.sql

pull:
	git pull
	git submodule update --recursive

update:
	-git pull
	-git commit -a
	git submodule update --init --recursive --remote
	git commit -a -m "Library update"

SQLlib/sqllib.o: SQLlib/sqllib.c
	git submodule update --recursive SQLLib
	make -C SQLlib
AJL/ajl.o: AJL/ajl.c
	git submodule update --recursive AJL
	make -C AJL

SQLINC=$(shell mariadb_config --include)
SQLLIB=$(shell mariadb_config --libs)
SQLVER=$(shell mariadb_config --version | sed 'sx\..*xx')
CCOPTS=${SQLINC} -I. -I/usr/local/ssl/include -D_GNU_SOURCE -g -Wall -funsigned-char -lm
OPTS=-L/usr/local/ssl/lib ${SQLLIB} ${CCOPTS}

tasconfig: tasconfig.c SQLlib/sqllib.o AJL/ajl.o
	cc -O -o $@ $< -lpopt -lmosquitto -ISQLlib SQLlib/sqllib.o -IAJL AJL/ajl.o ${OPTS}

