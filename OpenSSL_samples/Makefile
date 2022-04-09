all: digest.o


option = -L /home/hiroaki/openssl/ -l ssl -l crypto

digest.o: digest.cpp
	$(CXX) digest.cpp $(option) -o digest.o
