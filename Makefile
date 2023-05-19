all:
	g++ -std=c++17 ordering.cc -lsparsebase -fopenmp -lgomp -std=c++17 -o degree.out -DREORDER -DDEGREE
	g++ -std=c++17 ordering.cc -lsparsebase -fopenmp -lgomp -std=c++17 -o rcm.out -DREORDER -DRCM
	g++ -std=c++17 ordering.cc -lsparsebase -fopenmp -lgomp -std=c++17 -o gray.out -DREORDER -DGRAY
	g++ -std=c++17 ordering.cc -lsparsebase -fopenmp -lgomp -std=c++17 -o none.out -DNONE