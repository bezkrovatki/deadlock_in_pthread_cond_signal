CXXFLAGS += -std=c++14 -O2 -g
CXX ?= g++
multiarch := $(shell $(CXX) --print-multiarch)
PROG = test_pthread_cond_singal_deadlock.$(multiarch)

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
LDFLAGS += $(call if-expr-is-1,$(static_boost),$(static-link-push)) -lboost_date_time $(call if-expr-is-1,$(static_boost),$(static-link-pop)) -lpthread

all: $(PROG)

$(PROG): test.cpp makefile
	$(CXX) $(CXXFLAGS) -o "$@" $< $(LDFLAGS)

clean:
	rm $(PROG)
