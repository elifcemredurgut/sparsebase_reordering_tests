#include <stdio.h>
#include <vector>
#include "sparsebase/format/csr.h"

using namespace sparsebase;

template <
    typename id_type,
    typename nnz_type,
    typename value_type>
void CSRmv(
    format::CSR<id_type, nnz_type, value_type>&    a,
    nnz_type*    __restrict           row_ptr,    ///< Merge list A (row end-offsets)
    id_type*    __restrict            cols,
    value_type*     __restrict        vals,
    format::Array<value_type>&        vector_x,
    value_type*     __restrict        vector_y_out)
{
    for (int i=0; i<a.get_dimensions()[0]; ++i) {
        vector_y_out[i] = 0.0;
        for (int j=row_ptr[i]; j<row_ptr[i+1]; ++j)
            vector_y_out[i] += vals[j]*vector_x.get_vals()[cols[j]];
    }
}
