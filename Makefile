# compiler settings for debug/release builds
CXX=g++
CXXFLAGS=-Wall # -Wextra -Werror -DLINUX
CXX_DEBUG_FLAGS= -g3 # -DDEBUG_ALL
CXX_RELEASE_FLAGS=-O3
OPENMPFLAGS=-fopenmp
PROFILERFLAGS=-pg

# linker flags
LDFLAGS=-lfl

# change executable name / source files here
EXECUTABLE=planner
SOURCES=planner.cpp \
        PrettyPrint.cpp EpistemicModeling.cpp FormulaManager.cpp
DIRS=maepl search heuristics
DIROBS=maepl/data.o maepl/PlanningTask.o maepl/lex.yy.o \
        maepl/maepl.tab.o search/SearchAlgorithm.o \
        search/EAStar.o search/EAOStar.o heuristics/Heuristic.o \
        heuristics/Abstraction.o heuristics/Relaxation.o \
        heuristics/HStar.o

OBJECTS=$(SOURCES:.cpp=.o)

all: dirs $(SOURCES) $(EXECUTABLE)

.PHONY: dirs
dirs:
	for d in $(DIRS); do (cd $$d; $(MAKE) $(MFLAGS)); done
	
$(EXECUTABLE): dirs $(OBJECTS) 
	$(CXX) $(OBJECTS) $(DIROBS) $(LDFLAGS) -o $@

%.o: %.cpp *.h */*.h
	$(CXX) -c $(CXXFLAGS) $< -o $@

.PHONY: debug
debug: export CXXFLAGS+=$(CXX_DEBUG_FLAGS)
debug: all

.PHONY: profiling
profiling: export CXXFLAGS+=$(CXX_DEBUG_FLAGS) $(PROFILERFLAGS)
profiling: export LDFLAGS+=$(PROFILERFLAGS)
profiling: all

.PHONY: release
release: export CXXFLAGS+=$(CXX_RELEASE_FLAGS)
release: all

.PHONY: multicore
multicore: export CXXFLAGS+=$(CXX_RELEASE_FLAGS) $(OPENMPFLAGS)
multicore: export LDFLAGS+=$(OPENMPFLAGS)
multicore: all

.PHONY: clean
clean:
	rm -f *.o *~
	for d in $(DIRS); do (cd $$d; $(MAKE) clean); done

.PHONY: veryclean
veryclean: clean
	rm -f $(EXECUTABLE)


