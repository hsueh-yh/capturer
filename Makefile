
CC = g++


LIB_NDN = -lboost_thread -lboost_system -lboost_chrono -lboost_regex -lndn-cpp -lpthread -lprotobuf -lsqlite3 -lcrypto

LIB_FFMPEG = -lavutil -lavcodec -lavformat -lavdevice -lswscale 

LIBS = -lglog

PARAM = -g -O2 -std=c++11

TARGET = monitor

OBJECTS = utils.o frame-data.o namespacer.o name-components.o frame-buffer.o publisher.o 

all:$(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(PARAM) -o $(TARGET) main.cpp $(OBJECTS) $(INCLUDE) $(LIBDIR) $(LIB_NDN) $(LIB_FFMPEG) $(LIBS)

utils.o: frame-data.cpp
	$(CC) $(PARAM) -c utils.cpp

frame-data.o: frame-data.cpp
	$(CC) $(PARAM) -c frame-data.cpp

namespacer.o: namespacer.cpp
	$(CC) $(PARAM) -c namespacer.cpp

name-components.o: name-components.cpp
	$(CC) $(PARAM) -c name-components.cpp

frame-buffer.o: frame-buffer.cpp publisher.cpp
	$(CC) $(PARAM) -c frame-buffer.cpp

publisher.o: frame-buffer.o frame-data.o name-components.o
	$(CC) $(PARAM) -c $(INCLUDE)  publisher.cpp


cleangch:
	rm *.gch
	
clean:
	rm $(TARGET) *.gch
	
cleanall:
	rm *.o *.gch $(TARGET) *~
