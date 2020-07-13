CXXFLAGS += -std=c++14 -O2 -g
CXX ?= g++
PROG=test_asio_handlers_starvation2.arm64

ifneq ($(BOOST_HOME),)
    CXXFLAGS += -I$(BOOST_HOME)/include
    LDFLAGS += -L$(BOOST_HOME)/lib
endif

ifeq ($(static),1)
    LDFLAGS += -static
endif

if-expr-is-1 = $(if $(filter 1,$(1)),$(2),$(3))
static-link-push = '-Wl,-(' -Wl,--push-state -Wl,-Bstatic
static-link-pop = -Wl,--pop-state '-Wl,-)'
LDFLAGS += $(call if-expr-is-1,$(static_boost),$(static-link-push)) -lboost_date_time  -lboost_system -lboost_thread $(call if-expr-is-1,$(static_boost),$(static-link-pop)) -lpthread

all: $(PROG)

$(PROG): test.cpp makefile
	$(CXX) $(CXXFLAGS) -o "$@" $< $(LDFLAGS)

clean:
	rm $(PROG)
