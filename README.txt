Commands to test orderings:

make
./degree.out simple.mtx
./rcm.out simple.mtx
./gray.out simple.mtx

Some of the orderings do not compile. If you would like to test them, you can run the following commands:
g++ -std=c++17 ordering.cc -lsparsebase -fopenmp -lgomp -std=c++17 -o rabbit.out -DREORDER -DRABBIT
g++ -std=c++17 ordering.cc -lsparsebase -fopenmp -lgomp -std=c++17 -o generic.out -DREORDER -DGENERIC
g++ -std=c++17 ordering.cc -lsparsebase -fopenmp -lgomp -std=c++17 -o metis.out -DREORDER -DMETIS


simple.mtx                                         | simple2.mtx:
%---------------                                   | %------------
% | 2 0 0 |   | 1 |    | 2 |                       | % | 2 0 0 1 |   | 1 |   | 3  |
% | 0 5 3 | x | 1 | =  | 8 |                       | % | 0 5 3 1 | x | 1 | = | 9  |
% | 1 3 0 |   | 1 |    | 4 |                       | % | 1 3 0 1 |   | 1 |   | 5  |
%---------------                                   | % | 3 6 1 4 |   | 1 |   | 14 |

            with simple.mtx                        |     with simple2.mtx
rcm     --> compiles but wrong spmv result [4,2,2] | --> compiles but wrong spmv result  [9,5,14,3]
degree  --> compiles but wrong spmv result [4,8,2] | --> compiles but segmentation fault 
gray    --> compiles, correct              [2,8,4] | --> compiles but wrong spmv result  [14,9,5,3]
rabbit  --> does not compile                       |
generic --> does not compile                       |
metis   --> does not compile                       |
none    --> compiles, correct              [2,8,4] | --> compiles, correct               [3,9,5,14]  
