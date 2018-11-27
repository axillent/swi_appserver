PROJECT = swi_appserver

CPP = $(wildcard *.cpp)

LIBS = pthread event evhtp log4cpp mosquitto mosquittopp

CXX = g++

MYLIBS = /mnt/dev/lib.c/

FLAGS = -Wall -std=c++11 -O3 -g0 -I $(MYLIBS)

OBJ = $(CPP:.cpp=.o)

json_ext.o: json_ext.cpp $(MYLIBS)/smartletsp/common/comm_json_ext.hpp $(MYLIBS)/smartletsp/msg/msg_txt.hpp
	$(CXX) $(FLAGS) -c json_ext.cpp -o json_ext.o

%.o: %.cpp
	$(CXX) $(FLAGS) -c $^ -o $@

all: $(OBJ)
	$(CXX) $(FLAGS) $(OBJ) -o $(PROJECT) $(addprefix -l, $(LIBS))

clean:
	rm -f $(OBJ) $(PROJECT) *.o

rebuild: clean all

deploy: all
	./deploy.sh