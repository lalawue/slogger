#
#
# 

CLIENT_NAME = c.out
SERVER_NAME = s.out
LIB_NAME = slogger

SHARED_SUF = $(addsuffix .so, $(LIB_NAME))
SLIB_NAME = $(addprefix lib, $(SHARED_SUF))

SLIB_SOURCE = log_client.c log_xml.c log_shared.c  log_comm.c log_apd_std.c log_apd_file.c logc_apd_db.c
SERVER_SOURCE = log_server.c log_shared.c log_comm.c logs_apd_db.c
CLIENT_SOURCE = log_test.c


CFLAGS = -Wall -g -O2
SLIB_CFLAGS = ${CFLAGS} -shared

INCS_DIR = ./include
LIBS_DIR = ./ /usr/lib/mysql

CLIENT_LIBS = ${LIB_NAME}
SERVER_LIBS = mysqlclient
SLIB_LIBS = expat


SHARED_DEF = 
CLIENT_DEF = 

SRC_DIR= src/
TEST_DIR = test/

SLIB_SRC_LIST = $(addprefix $(SRC_DIR), $(SLIB_SOURCE))
SERVER_SRC_LIST = $(addprefix $(SRC_DIR), $(SERVER_SOURCE))
CLIENT_SRC_LIST = $(addprefix $(TEST_DIR), $(CLIENT_SOURCE))

INCS_LIST = $(addprefix -I, $(INCS_DIR))
LIB_PATHS = $(addprefix -L, $(LIBS_DIR))

CLIENT_LIB_LIST = $(addprefix -l, $(CLIENT_LIBS))
SERVER_LIB_LIST = $(addprefix -l, $(SERVER_LIBS))
SLIB_LIST = $(addprefix -l, $(SLIB_LIBS))


SHARED_DEF_LIST = $(addprefix -D, $(SHARED_DEF))
CLIENT_DEF_LIST = $(addprefix -D, $(CLIENT_DEF))


all:
	gcc -o $(SLIB_NAME) $(SLIB_CFLAGS) $(SLIB_LIST) $(SHARED_DEF_LIST) $(SLIB_SRC_LIST) $(INCS_LIST)
	gcc -o $(SERVER_NAME) $(CFLAGS) $(LIB_PATHS) $(SERVER_LIB_LIST) $(SERVER_SRC_LIST)  $(INCS_LIST)
	gcc -o $(CLIENT_NAME) $(CFLAGS) $(CLIENT_SRC_LIST) $(LIB_PATHS) $(CLIENT_LIB_LIST) $(INCS_LIST)


.PHONY: clean

clean:
	rm -f *.out *.o *.so ${CLIENT_NAME} ${SERVER_NAME} ${SLIB_LIBS}