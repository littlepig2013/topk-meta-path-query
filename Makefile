CXX = g++
RM = -rm -f
CXXFLAGS = -std=c++11 -g -pthread  #-fsanitize=address
LDLIBS =-I ~/include/

OTHER_OBJECTS = AppUtils.o CommonUtils.o HIN_Graph.o \
	SimCalculator.o TopKCalculator.o yagoReader.o

all: topKQuery topKQueryTest genTopKMPs genRandomWalk

topKQuery: $(OTHER_OBJECTS)
	$(CXX) $(CXXFLAGS) -o topKQuery $(OTHER_OBJECTS) main.cpp

topKQueryTest: $(OTHER_OBJECTS)
	$(CXX) $(CXXFLAGS) -o topKQueryTest $(OTHER_OBJECTS) test.cpp 

genTopKMPs: $(OTHER_OBJECTS)
	$(CXX) $(CXXFLAGS) -o genTopKMPs $(OTHER_OBJECTS) genTopKMPs.cpp

genRandomWalk: $(OTHER_OBJECTS)
	$(CXX) $(CXXFLAGS) -o genRandomWalk $(OTHER_OBJECTS) genRandomWalk.cpp


clean:
	$(RM) topKQuery topKQueryTest genTopKMPs genRandomWalk $(OTHER_OBJECTS)



