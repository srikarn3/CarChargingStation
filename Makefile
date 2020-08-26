CFLAGS = -Wall -g
LIBS = -lpthread

all: ccs_server ccs_client

ccs_server: ccs_includes.hpp ccs_server.cpp
	$(CXX) $(CFLAGS) $? -o $@

ccs_client: ccs_includes.hpp ccs_client.cpp
	$(CXX) $(CFLAGS) $? -o $@

.PHONY: clean
clean:
	rm -f ccs_server ccs_client
