
CXXFLAGS=-Wall -I. -std=c++2a -Wfatal-errors -I$(SIMPATH)/include -g

LDFLAGS=-L$(SIMPATH)/lib -lyaml-cpp -lFairLogger
LD=g++

%.o: %.cxx
	g++ -c $(CXXFLAGS) $< -o $@
sanepar: sanepar.o main.o
	g++ $(CXXFLAGS) $(LDFLAGS) $^ -o $@

#	g++ -std=c++2a sanepar.cxx -I. -Wfatal-errors -I /u/land/klenze/FairRoot/fairtools/ $(root-config --cflags) -I /u/land/klenze/2022-06/inst/FairSoft/include/   /u/land/klenze/2022-06/inst/FairSoft/lib/libFairLogger.so -l yaml-cpp -g
clean:
	rm *.o
