#include "sparsebase/format/csr.h"
#include "sparsebase/bases/iobase.h"
#include "sparsebase/bases/reorder_base.h"
#include "sparsebase/reorder/rcm_reorder.h"
#include "sparsebase/reorder/degree_reorder.h"
#include "sparsebase/reorder/metis_reorder.h"
#include "sparsebase/reorder/gray_reorder.h"
#include "sparsebase/reorder/generic_reorder.h"
#include "sparsebase/reorder/rabbit_reorder.h"
#include "sparsebase/context/cpu_context.h"
#include <string>
#include <iostream>
//#include "mv.hpp"
#include "spmv.hpp"

typedef unsigned int id_type;
typedef unsigned int nnz_type;
typedef double value_type;

using namespace sparsebase;
using namespace io;
using namespace bases;
using namespace reorder;
using namespace format;

int main(int argc, char * argv[]){
    if (argc < 2){
        std::cout << "Please enter the name of the matrix file as a parameter\n";
        return 1;
    }
    // The name of the edge list file in disk
    std::string filename(argv[1]);
    // Read the edge list file into a CSR object
    //CSR<id_type, nnz_type, value_type>* csr = IOBase::ReadMTXToCSR<id_type, nnz_type, value_type>(filename);
    CSR<id_type, nnz_type, value_type> *csr = MTXReader<id_type, nnz_type, value_type>(filename).ReadCSR();

    std::cout << "Original graph:" << std::endl;
    // get a array representing the dimensions of the matrix represented by `csr`,
    // i.e, the adjacency matrix of the graph
    std::cout << "Number of vertices: " << csr->get_dimensions()[0] << std::endl;
    // Number of non-zeros in the matrix represented by `csr`
    std::cout << "Number of edges: " << csr->get_num_nnz() << std::endl;

    // Create a CPU context
    context::CPUContext cpu_context;
    // We would like to order the vertices by degrees in descending order
    bool ascending = false;

    #ifdef REORDER
    #ifdef DEGREE
    DegreeReorderParams params(ascending);
    // Create a permutation array of `csr` using one of the passed contexts
    // (in this case, only one is passed)
    // The last argument tells the function to convert the input format if needed
    id_type* new_order = ReorderBase::Reorder<DegreeReorder>(params, csr, {&cpu_context}, true);
    #endif

    #ifdef RCM
    RCMReorderParams params;
    // Create a permutation array of `csr` using one of the passed contexts
    // (in this case, only one is passed)
    // The last argument tells the function to convert the input format if needed
    id_type* new_order = ReorderBase::Reorder<RCMReorder>(params, csr, {&cpu_context}, true);
    #endif

    #ifdef GRAY
    GrayReorderParams params(BitMapSize::BitSize32, 32, 32);
    // Create a permutation array of `csr` using one of the passed contexts
    // (in this case, only one is passed)
    // The last argument tells the function to convert the input format if needed
    id_type* new_order = ReorderBase::Reorder<GrayReorder>(params, csr, {&cpu_context}, true);
    #endif

    #ifdef METIS
    MetisReorderParams params;
    auto global_csr_64_bit = csr.Convert<sparsebase::format::CSR, int64_t, int64_t, int64_t>(false);
    // Create a permutation array of `csr` using one of the passed contexts
    // (in this case, only one is passed)
    // The last argument tells the function to convert the input format if needed
    id_type* new_order = ReorderBase::Reorder<MetisReorder>(params, global_csr_64_bit, {&cpu_context}, true);
    #endif

    #ifdef GENERIC    
    // Create a permutation array of `csr` using one of the passed contexts
    // (in this case, only one is passed)
    // The last argument tells the function to convert the input format if needed
    id_type* new_order = ReorderBase::Reorder<GenericReorder>(csr, {&cpu_context}, true);
    #endif

    #ifdef RABBIT
    //MetisReorderParams params;
    // Create a permutation array of `csr` using one of the passed contexts
    // (in this case, only one is passed)
    // The last argument tells the function to convert the input format if needed
    id_type* new_order = ReorderBase::Reorder<RabbitReorder>({}, &csr, {&cpu_context}, true);
    #endif

    // Permute2D permutes the rows and columns of `csr` according to `new_order`
    // Similar to `Reorder`, we specify the contexts to use,
    // and whether the library can convert the input if needed
    FormatOrderTwo<id_type, nnz_type, value_type>* new_format = ReorderBase::Permute2D(new_order, csr, {&cpu_context}, true);
    // Cast the polymorphic pointer to a pointer at CSR
    CSR<id_type, nnz_type, value_type>* new_csr = new_format->As<CSR>();
    #endif

    #ifdef NONE
    CSR<id_type, nnz_type, value_type>* new_csr = csr;
    #endif
    
    std::cout << "ordering is done\n";
    std::cout << "Reordered graph:" << std::endl;
    std::cout << "Number of vertices: " << new_csr->get_dimensions()[0] << std::endl;
    std::cout << "Number of edges: " << new_csr->get_num_nnz() << std::endl;
    

    double *vector_x, *vector_y_out;
    
    vector_x                = (double*) calloc(csr->get_dimensions()[1], sizeof(double));
    vector_y_out            = (double*) calloc(csr->get_dimensions()[0], sizeof(double));

    //fill the vector_x with zeros
    for (int col = 0; col < new_csr->get_dimensions()[1]; ++col)
        vector_x[col] = 1.0;

    //Merge-base SPMV
    //OmpMergeCsrmv<double, unsigned long long, long long unsigned int>(1, *csr, csr->get_row_ptr(), csr->get_col(), csr->get_vals(), vector_x, vector_y_out);
    
    //Simple SPMV
    CSRmv<double, unsigned int, unsigned int>(*new_csr, new_csr->get_row_ptr(), new_csr->get_col(), new_csr->get_vals(), vector_x, vector_y_out);
    std::cout << "spmv is done\n" << std::endl;

    //uncomment this part if you want to see the spmv results
    /*
    std::cout << "\nVector y:\n";
    for (int row = 0; row < new_csr->get_dimensions()[0]; ++row)
        std::cout << vector_y_out[row] << " ";
    std::cout << "\n";
    */
    free(vector_x);
    free(vector_y_out);
    std::cout << "freed vectors" << std::endl;
    
    return 0;
}