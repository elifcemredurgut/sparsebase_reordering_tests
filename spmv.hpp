#include <stdio.h>
#include <vector>
#include <sparsebase/format/format.h>
#include <sparsebase/object/object.h>
#include <sparsebase/preprocess/preprocess.h>
#include <sparsebase/utils/io/reader.h>

using namespace sparsebase;

template <
    typename ValueType,
    typename NNZType,
    typename IDType>
void CSRmv(
    format::CSR<unsigned long long, unsigned long long, double>&    a,
    NNZType*    __restrict           row_ptr,    ///< Merge list A (row end-offsets)
    IDType*    __restrict            cols,
    ValueType*     __restrict        vals,
    ValueType*     __restrict        vector_x,
    ValueType*     __restrict        vector_y_out)
{
    for (int i=0; i<a.get_dimensions()[0]; ++i) {
        vector_y_out[i] = 0.0;
        for (int j=row_ptr[i]; j<row_ptr[i+1]; ++j)
            vector_y_out[i] += vals[j]*vector_x[cols[j]];
    }
}
