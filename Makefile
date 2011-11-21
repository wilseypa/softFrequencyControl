# Dependencies:
#  - pthread
#  - GNU Scientific Library
#  - Boost program options

SRC = src
INCLUDE = -I include
OBJ = obj
BIN = bin

ROOT = throttling_tests

CXXFLAGS = -Wall -O3
LIBS = -lpthread -lrt -lboost_program_options -lgsl -lgslcblas


NANO_TIME = 0

THROT1 = $(SRC)/tests/throt_1.cpp
THROT2 = $(SRC)/tests/throt_2.cpp
THROT3 = $(SRC)/tests/throt_3.cpp
THROT4 = $(SRC)/tests/throt_4.cpp
THROTC = $(SRC)/tests/throt_controlled.cpp

CPUFUNC = $(SRC)/utils/cpufunc.cpp
CPUFUNC_OBJ = $(OBJ)/cpufunc.o

TIME = $(SRC)/utils/timing.cpp
TIME_OBJ = $(OBJ)/timing.o

LOGGING = $(SRC)/utils/logging.cpp
LOGGING_OBJ = $(OBJ)/logging.o

PROCBIND = $(SRC)/utils/procbind.cpp
PROCBIND_OBJ = $(OBJ)/procbind.o

OBJS = $(TIME_OBJ) \
	$(LOGGING_OBJ) \
	$(PROCBIND_OBJ) \
	$(CPUFUNC_OBJ)

DIR = directory

TEST1 = $(BIN)/Throttling1
TEST2 = $(BIN)/Throttling2
TEST3 = $(BIN)/Throttling3
TEST4 = $(BIN)/Throttling4
THROT_CTRL = $(BIN)/ThrotCtrl

TESTS = $(TEST1) \
	$(TEST3) \
    $(THROT_CTRL)

test: $(DIR) $(TESTS)

$(DIR) : 
	@[ -d $(BIN) ] || mkdir -p $(BIN)
	@[ -d $(OBJ) ] || mkdir $(OBJ)

# Utility functions object Files

$(CPUFUNC_OBJ) : $(CPUFUNC)
	$(CXX) $(INCLUDE) -c $(CPUFUNC) -o $@ $(LIBS)

$(TIME_OBJ) : $(TIME)
	$(CXX) $(INCLUDE) -c $(TIME) -D NANO_TIME=$(NANO_TIME) -o $@ $(LIBS)

$(LOGGING_OBJ) : $(LOGGING)
	$(CXX) $(INCLUDE) -c $(LOGGING) -D NANO_TIME=$(NANO_TIME) -o $@ $(LIBS)

$(PROCBIND_OBJ) : $(PROCBIND)
	$(CXX) $(INCLUDE) -c $(PROCBIND) -D NANO_TIME=$(NANO_TIME) -o $@ $(LIBS)

# Test Program Compilations

$(TEST1):  $(OBJS) $(THROT1)
	$(CXX) $(INCLUDE) $(CXXFLAGS) $(THROT1) -D NANO_TIME=$(NANO_TIME) -o $@ $(OBJS) $(LIBS)

$(TEST2) : $(OBJS) $(THROT2)
	$(CXX) $(INCLUDE) $(CXXFLAGS) $(THROT2) -D NANO_TIME=$(NANO_TIME) -o $@ $(OBJS) $(LIBS)

$(TEST3) : $(OBJS) $(THROT3)
	$(CXX) $(INCLUDE) $(CXXFLAGS) $(THROT3) -D NANO_TIME=$(NANO_TIME) -o $@ $(OBJS) $(LIBS)

$(TEST4) : $(OBJS) $(THROT4)
	$(CXX) $(INCLUDE) $(CXXFLAGS) $(THROT4) -D NANO_TIME=$(NANO_TIME) -o $@ $(OBJS) $(LIBS)

$(THROT_CTRL) : $(OBJS) $(THROTC)
	$(CXX) $(INCLUDE) $(CXXFLAGS) $(THROTC) -D NANO_TIME=$(NANO_TIME) -o $@ $(OBJS) $(LIBS)

clean:
	rm $(TESTS) $(OBJS)

