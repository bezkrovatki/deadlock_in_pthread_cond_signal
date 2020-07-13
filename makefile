LDFLAGS = -lboost_date_time -lboost_system -lboost_thread -lpthread 
CXXFLAGS = -std=c++14 -O2 -g
CXX ?= g++
PROG=test_asio_handlers_starvation2.arm64

ifneq ($(BOOST_HOME),)
    CXXFLAGS += -I$(BOOST_HOME)/include
    LDFLAGS += -L$(BOOST_HOME)/lib
endif

ifeq ($(static),1)
    LDFLAGS += -static
endif

all: $(PROG)

$(PROG): test.cpp makefile
	$(CXX) $(CXXFLAGS) -o "$@" $< $(LDFLAGS)

clean:
	rm $(PROG)
