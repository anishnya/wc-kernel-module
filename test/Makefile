CXX=g++
CXXFLAGS=-std=c++20 -Wall -Ofast -mavx512f -mavx512vl -march=native
LDFLAGS=-pthread

all: wc_test wc_test_shared shm_benchmark

wc_test: wc_test.cpp constants.h stats.hpp bench.hpp mem.hpp
	$(CXX) $(CXXFLAGS) wc_test.cpp -o wc_test

wc_test_shared: wc_test_shared.cpp constants.h stats.hpp bench.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) wc_test_shared.cpp -o wc_test_shared

shm_benchmark: shm_benchmark.cpp constants.h stats.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) shm_benchmark.cpp -o shm_benchmark

clean:
	-@ rm -f wc_test wc_test_shared shm_benchmark *~ *.txt >/dev/null 2>&1

