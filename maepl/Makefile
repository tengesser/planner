CC=g++

OBJECTS=data.o PlanningTask.o lex.yy.o maepl.tab.o

all: $(OBJECTS)

%.o: %.cpp *.h ../*.h ../*/*.h

maepl.tab.c maepl.tab.h: maepl.y maepl.h
	bison -d maepl.y

lex.yy.c: maepl.l maepl.tab.h
	flex maepl.l

clean:
	rm -f *.o *~ maepl.tab.c maepl.tab.h lex.yy.c


