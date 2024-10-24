# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -Wextra -std=c++11

# Source files
SEND_SRC = NameServer.cpp dijkstra.cpp helpers.cpp
RECV_SRC = miProxy.cpp miProxyData.cpp miProxyEval.cpp helpers.cpp

# Output executables
DNS_EXE = nameserver
PROXY_EXE = miProxy

# Build DNS executable
$(DNS_EXE): $(DNS_SRC)
        $(CXX) $(CXXFLAGS) -o $@ $^

# Build Proxy executable
$(PROXY_EXE): $(PROXY_SRC)
        $(CXX) $(CXXFLAGS) -o $@ $^

all: ${DNS_EXE} ${PROXY_EXE}

# Clean rule to remove compiled files
clean:
        rm -f $(DNS_EXE) $(PROXY_EXE)

# PHONY to avoid conflicts with any files named 'clean'
.PHONY: all clean