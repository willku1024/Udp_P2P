



#SOURCES = $(notdir $(wildcard src/*))
#HEADERS = $(wildcard inc/*)

TOP_DIR = $(shell pwd)
OBJ_DIR = $(TOP_DIR)/obj
BIN_DIR = $(TOP_DIR)/bin
HDR_DIR = $(TOP_DIR)/inc
SRC_DIR = $(TOP_DIR)/src

SOURCES =  $(wildcard $(SRC_DIR)/*)
OBJFILES = $(SOURCES:%.cc=%.o)

CC = g++
FLAGS = -std=c++17 -Wall -I $(HDR_DIR)
LIBS += -lpthread -lprotobuf
CLIENT = client
SERVER = server



all:$(OBJFILES) $(CLIENT) $(SERVER)


$(CLIENT):
	@echo "\033[33m build client \033[0m" 
	@echo $(C_FILES)
	$(CC) $(filter-out %$(join $(SERVER), .o), $(wildcard $(OBJ_DIR)/*)) -o $(BIN_DIR)/$@ $(LIBS)

$(SERVER):
	@echo "\033[33m build server \033[0m" 
	@echo $(C_FILES)
	$(CC) $(filter-out %$(join $(CLIENT), .o), $(wildcard $(OBJ_DIR)/*)) -o $(BIN_DIR)/$@ $(LIBS)


$(OBJFILES): %.o:%.cc 
	@echo "\033[33m build objects \033[0m" 
	$(CC) $(FLAGS) -c $< -o $(OBJ_DIR)/$(notdir $@) 

clean:
	-rm -rf $(TOP_DIR)/obj/* 
	-rm -rf $(TOP_DIR)/bin/* 


.PHONY:clean all install
