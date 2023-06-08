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
#include "mv.hpp"
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
    for (int i = 1; i < 2; i++){
    if (argc < 2){
        std::cout << "Please enter the name of the matrix file as a parameter\n";
        return 1;
    }
    std::string filename(argv[1]);
    //CSR<id_type, nnz_type, value_type>* csr = IOBase::ReadMTXToCSR<id_type, nnz_type, value_type>(filename);
    CSR<id_type, nnz_type, value_type> *csr = MTXReader<id_type, nnz_type, value_type>(filename).ReadCSR();
    std::cout << "reading is done\n";
    /*
    std::cout << "Original graph:" << std::endl;
    // get a array representing the dimensions of the matrix represented by `csr`,
    // i.e, the adjacency matrix of the graph
    std::cout << "Number of vertices: " << csr->get_dimensions()[0] << std::endl;
    // Number of non-zeros in the matrix represented by `csr`
    std::cout << "Number of edges: " << csr->get_num_nnz() << std::endl;
    */
    // Create a CPU context
    context::CPUContext cpu_context;
    // We would like to order the vertices by degrees in descending order
    bool ascending = false;

    double *vector_y_out;
    vector_y_out            = (double*) calloc(csr->get_dimensions()[0], sizeof(double));

    value_type * vector_x = new value_type[csr->get_dimensions()[1]];  //num of columns
    //fill the vector_x with 1's
    for (int col = 0; col < csr->get_dimensions()[1]; ++col)
        vector_x[col] = 1.0;

    Array<value_type>* vec_ptr = new Array<value_type>(csr->get_dimensions()[1], vector_x, kOwned);

    #ifdef REORDER
    #ifdef DEGREE
    DegreeReorderParams params(ascending);
    id_type* new_order = ReorderBase::Reorder<DegreeReorder>(params, csr, {&cpu_context}, true);
    #endif

    #ifdef RCM
    RCMReorderParams params;
    id_type* new_order = ReorderBase::Reorder<RCMReorder>(params, csr, {&cpu_context}, true);
    #endif

    #ifdef GRAY
    GrayReorderParams params(BitMapSize::BitSize32, 32, 32);
    id_type* new_order = ReorderBase::Reorder<GrayReorder>(params, csr, {&cpu_context}, true);
    #endif

    #ifdef METIS
    MetisReorderParams params;
    auto global_csr_64_bit = csr.Convert<sparsebase::format::CSR, int64_t, int64_t, int64_t>(false);
    id_type* new_order = ReorderBase::Reorder<MetisReorder>(params, global_csr_64_bit, {&cpu_context}, true);
    #endif

    #ifdef GENERIC    
    id_type* new_order = ReorderBase::Reorder<GenericReorder>(csr, {&cpu_context}, true);
    #endif

    #ifdef RABBIT
    id_type* new_order = ReorderBase::Reorder<RabbitReorder>({}, &csr, {&cpu_context}, true);
    #endif

    // Permute2D permutes the rows and columns of `csr` according to `new_order`
    // Similar to `Reorder`, we specify the contexts to use,
    // and whether the library can convert the input if needed
    FormatOrderTwo<id_type, nnz_type, value_type>* new_format = ReorderBase::Permute2D(new_order, csr, {&cpu_context}, true);
    // Cast the polymorphic pointer to a pointer at CSR
    CSR<id_type, nnz_type, value_type>* new_csr = new_format->As<CSR>();
    std::cout << "csr ordering is done\n";

    Array<value_type>* new_vec = ReorderBase::Permute1D<Array>(new_order, vec_ptr, {&cpu_context}, true);
    Array<value_type>* vec = new_vec->As<Array>();
    std::cout << "vector ordering is done\n";
    #endif

    #ifdef NONE
    CSR<id_type, nnz_type, value_type>* new_csr = csr;
    Array<value_type>* vec = vec_ptr;
    #endif

    //Merge-base SPMV
    //OmpMergeCsrmv<value_type, nnz_type, id_type>(1, *new_csr, new_csr->get_row_ptr(), new_csr->get_col(), new_csr->get_vals(), vec, vector_y_out);
    
    //Simple SPMV
    CSRmv<id_type, nnz_type, value_type>(*new_csr, new_csr->get_row_ptr(), new_csr->get_col(), new_csr->get_vals(), *vec, vector_y_out);
    std::cout << "spmv is done" << std::endl;

    //comment this part if you test with larger matrices
    
    std::cout << "\nFor simple.mtx, vector y should be:\n[2 8 4 ]\n";
    std::cout << "Vector y:\n[";
    for (int row = 0; row < new_csr->get_dimensions()[0]; ++row)
        std::cout << vector_y_out[row] << " ";
    std::cout << "]\n";
    
    free(vector_x);
    free(vector_y_out);
    //std::cout << "freed vectors" << std::endl;
    }
    return 0;
}