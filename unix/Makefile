# makefile for programs and generator

CCOMP = c++
CFLAGS_DEBUG = -g
CFLAGS_RELEASE = -O5 -DNDEBUG
CFLAGS = $(CFLAGS_RELEASE) -I../energy -I../maxflow 
CODE_DIR = ../code
ENERGY_DIR = ../energy
MAXFLOW_DIR = ../maxflow
OUTPUT = ../bin/match

all: $(OUTPUT)

$(OUTPUT): main.o match.o data.o statistics.o kz2.o image.o graph.o maxflow.o
	$(CCOMP) $(CFLAGS) -o $(OUTPUT) main.o match.o data.o statistics.o kz2.o image.o graph.o maxflow.o

main.o: $(CODE_DIR)/main.cpp $(CODE_DIR)/match.h $(CODE_DIR)/image.h
	$(CCOMP) $(CFLAGS) -c -o main.o $(CODE_DIR)/main.cpp

match.o: $(CODE_DIR)/match.cpp $(CODE_DIR)/match.h $(CODE_DIR)/image.h 
	$(CCOMP) $(CFLAGS) -c -o match.o $(CODE_DIR)/match.cpp

statistics.o: $(CODE_DIR)/statistics.cpp $(CODE_DIR)/match.h $(CODE_DIR)/image.h
	$(CCOMP) $(CFLAGS) -c -o statistics.o $(CODE_DIR)/statistics.cpp

data.o: $(CODE_DIR)/data.cpp $(CODE_DIR)/match.h $(CODE_DIR)/image.h
	$(CCOMP) $(CFLAGS) -c -o data.o $(CODE_DIR)/data.cpp

kz2.o: $(CODE_DIR)/kz2.cpp $(CODE_DIR)/match.h $(CODE_DIR)/image.h $(ENERGY_DIR)/energy.h $(MAXFLOW_DIR)/graph.h $(MAXFLOW_DIR)/block.h
	$(CCOMP) $(CFLAGS) -c -o kz2.o $(CODE_DIR)/kz2.cpp

image.o: $(CODE_DIR)/image.cpp $(CODE_DIR)/image.h
	$(CCOMP) $(CFLAGS) -c -o image.o $(CODE_DIR)/image.cpp

graph.o: $(MAXFLOW_DIR)/graph.cpp $(MAXFLOW_DIR)/graph.h $(MAXFLOW_DIR)/block.h
	$(CCOMP) $(CFLAGS) -c -o graph.o $(MAXFLOW_DIR)/graph.cpp

maxflow.o: $(MAXFLOW_DIR)/maxflow.cpp $(MAXFLOW_DIR)/graph.h $(MAXFLOW_DIR)/block.h
	$(CCOMP) $(CFLAGS) -c -o maxflow.o $(MAXFLOW_DIR)/maxflow.cpp

clean:
	rm -rf $(OUTPUT)
	rm -rf *.o


