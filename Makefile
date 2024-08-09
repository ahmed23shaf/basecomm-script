CXX = g++
CXXFLAGS = -g -Wall

PARSER_DIR = src/parser
OBJECTS_DIR = src/objects
OBJECTS_DIR_WIN = src\objects
PUGI_DIR = src/lib/pugi
SERIAL_DIR = src/serial
EASYLOGGING_DIR = src/lib/easylogging
LOGGER_DIR = src/logger
LOGS_DIR = logs

## Enforce directories exist
ifeq ($(OS),Windows_NT)
	CREATE_OBJ_CMD = if not exist $(OBJECTS_DIR_WIN) mkdir $(OBJECTS_DIR_WIN)
	CREATE_LOGS_CMD = if not exist $(LOGS_DIR) mkdir $(LOGS_DIR)
else
	CREATE_OBJ_CMD = mkdir -p $(OBJECTS_DIR)
	CREATE_LOGS_CMD = mkdir -p $(LOGS_DIR)
endif

## Main executable
main: dirs $(OBJECTS_DIR)/main.o $(OBJECTS_DIR)/msg_field.o $(OBJECTS_DIR)/msg.o $(PUGI_DIR)/pugixml.o $(OBJECTS_DIR)/msg_table.o \
	$(OBJECTS_DIR)/xml_handler.o $(OBJECTS_DIR)/lf_comm.o $(OBJECTS_DIR)/easylogging++.o $(OBJECTS_DIR)/log.o
	$(CXX) $(CXXFLAGS) -o main $(OBJECTS_DIR)/main.o $(OBJECTS_DIR)/msg_field.o $(OBJECTS_DIR)/msg.o $(PUGI_DIR)/pugixml.o $(OBJECTS_DIR)/msg_table.o \
	$(OBJECTS_DIR)/xml_handler.o $(OBJECTS_DIR)/lf_comm.o $(OBJECTS_DIR)/easylogging++.o $(OBJECTS_DIR)/log.o

dirs:
	$(CREATE_OBJ_CMD)
	$(CREATE_LOGS_CMD)

#### Compiling source files ####
$(OBJECTS_DIR)/main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp -o $(OBJECTS_DIR)/main.o

$(OBJECTS_DIR)/msg_field.o: $(PARSER_DIR)/msg_field.cpp $(PARSER_DIR)/msg_field.h
	$(CXX) $(CXXFLAGS) -c $(PARSER_DIR)/msg_field.cpp -o $(OBJECTS_DIR)/msg_field.o

$(OBJECTS_DIR)/msg.o: $(PARSER_DIR)/msg.cpp $(PARSER_DIR)/msg.h
	$(CXX) $(CXXFLAGS) -c $(PARSER_DIR)/msg.cpp -o $(OBJECTS_DIR)/msg.o

$(OBJECTS_DIR)/msg_table.o: $(PARSER_DIR)/msg_table.cpp $(PARSER_DIR)/msg_table.h
	$(CXX) $(CXXFLAGS) -c $(PARSER_DIR)/msg_table.cpp -o $(OBJECTS_DIR)/msg_table.o

$(OBJECTS_DIR)/xml_handler.o: $(PARSER_DIR)/xml_handler.cpp $(PARSER_DIR)/xml_handler.h
	$(CXX) $(CXXFLAGS) -c $(PARSER_DIR)/xml_handler.cpp -o $(OBJECTS_DIR)/xml_handler.o

$(OBJECTS_DIR)/lf_comm.o: $(SERIAL_DIR)/lf_comm.cpp $(SERIAL_DIR)/lf_comm.h
	$(CXX) $(CXXFLAGS) -c $(SERIAL_DIR)/lf_comm.cpp -o $(OBJECTS_DIR)/lf_comm.o

$(OBJECTS_DIR)/easylogging++.o: $(EASYLOGGING_DIR)/easylogging++.cc $(EASYLOGGING_DIR)/easylogging++.h
	$(CXX) -g -DELPP_NO_DEFAULT_LOG_FILE -c $(EASYLOGGING_DIR)/easylogging++.cc -o $(OBJECTS_DIR)/easylogging++.o

$(OBJECTS_DIR)/log.o: $(LOGGER_DIR)/log.cpp $(LOGGER_DIR)/log.h
	$(CXX) -g -c $(LOGGER_DIR)/log.cpp -o $(OBJECTS_DIR)/log.o

# Prevent a 'clean.o/clean.exe' file
.PHONY: clean dirs

## Cleaning
clean:
ifeq ($(OS),Windows_NT)
	del /s /q *.o *.exe
else
	find . -name "*.o" -type f -delete
	find . -name "main" -type f -delete
endif
