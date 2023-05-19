Commands to test orderings:

make
./degree.out simple.mtx
./rcm.out simple.mtx
./gray.out simple.mtx

Some of the orderings do not compile. If you would like to test them, you can run the following commands:
g++ -std=c++17 ordering.cc -lsparsebase -fopenmp -lgomp -std=c++17 -o rabbit.out -DREORDER -DRABBIT
g++ -std=c++17 ordering.cc -lsparsebase -fopenmp -lgomp -std=c++17 -o generic.out -DREORDER -DGENERIC
g++ -std=c++17 ordering.cc -lsparsebase -fopenmp -lgomp -std=c++17 -o metis.out -DREORDER -DMETIS
