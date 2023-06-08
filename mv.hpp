//#include <omp.h>
#include <stdio.h>
#include <vector>
#include <sparsebase/format/csr.h>
using namespace sparsebase;

//---------------------------------------------------------------------
// Utility types
//---------------------------------------------------------------------

struct int2
{
    int x;
    int y;
};



/**
 * Counting iterator
 */
template <
    typename ValueType,
    typename IDType = ptrdiff_t>
struct CountingInputIterator
{
    // Required iterator traits
    typedef CountingInputIterator               self_type;              ///< My own type
    typedef IDType                             difference_type;        ///< Type to express the result of subtracting one iterator from another
    typedef ValueType                           value_type;             ///< The type of the element the iterator can point to
    typedef ValueType*                          pointer;                ///< The type of a pointer to an element the iterator can point to
    typedef ValueType                           reference;              ///< The type of a reference to an element the iterator can point to
    typedef std::random_access_iterator_tag     iterator_category;      ///< The iterator category

    ValueType val;

    /// Constructor
    inline CountingInputIterator(
        const ValueType &val)          ///< Starting value for the iterator instance to report
    :
        val(val)
    {}

    /// Postfix increment
    inline self_type operator++(int)
    {
        self_type retval = *this;
        val++;
        return retval;
    }

    /// Prefix increment
    inline self_type operator++()
    {
        val++;
        return *this;
    }

    /// Indirection
    inline reference operator*() const
    {
        return val;
    }

    /// Addition
    template <typename Distance>
    inline self_type operator+(Distance n) const
    {
        self_type retval(val + n);
        return retval;
    }

    /// Addition assignment
    template <typename Distance>
    inline self_type& operator+=(Distance n)
    {
        val += n;
        return *this;
    }

    /// Subtraction
    template <typename Distance>
    inline self_type operator-(Distance n) const
    {
        self_type retval(val - n);
        return retval;
    }

    /// Subtraction assignment
    template <typename Distance>
    inline self_type& operator-=(Distance n)
    {
        val -= n;
        return *this;
    }

    /// Distance
    inline difference_type operator-(self_type other) const
    {
        return val - other.val;
    }

    /// Array subscript
    template <typename Distance>
    inline reference operator[](Distance n) const
    {
        return val + n;
    }

    /// Structure dereference
    inline pointer operator->()
    {
        return &val;
    }

    /// Equal to
    inline bool operator==(const self_type& rhs)
    {
        return (val == rhs.val);
    }

    /// Not equal to
    inline bool operator!=(const self_type& rhs)
    {
        return (val != rhs.val);
    }

    /// ostream operator
    friend std::ostream& operator<<(std::ostream& os, const self_type& itr)
    {
        os << "[" << itr.val << "]";
        return os;
    }
};


//---------------------------------------------------------------------
// MergePath Search
//---------------------------------------------------------------------


/**
 * Computes the begin offsets into A and B for the specific diagonal
 */
template <
    typename AIteratorT,
    typename BIteratorT,
    typename IDType,
    typename CoordinateT>
inline void MergePathSearch(
    IDType         diagonal,           ///< [in]The diagonal to search
    AIteratorT      a,                  ///< [in]List A
    BIteratorT      b,                  ///< [in]List B
    IDType         a_len,              ///< [in]Length of A
    IDType         b_len,              ///< [in]Length of B
    CoordinateT&    path_coordinate)    ///< [out] (x,y) coordinate where diagonal intersects the merge path
{
    IDType x_min = std::max(diagonal - b_len, 0);
    IDType x_max = std::min(diagonal, a_len);

    while (x_min < x_max)
    {
        IDType x_pivot = (x_min + x_max) >> 1;
        if (a[x_pivot] <= b[diagonal - x_pivot - 1])
            x_min = x_pivot + 1;    // Contract range up A (down B)
        else
            x_max = x_pivot;        // Contract range down A (up B)
    }

    path_coordinate.x = std::min(x_min, a_len);
    path_coordinate.y = diagonal - x_min;
}


template <
    typename ValueType,
    typename NNZType,
    typename IDType>
void OmpMergeCsrmv(
    int                              num_threads,
    format::CSR<IDType, NNZType, ValueType>&    a,
    NNZType*    __restrict           row_ptr,    ///< Merge list A (row end-offsets)
    IDType*    __restrict            cols,
    ValueType*     __restrict        vals,
    format::Array<ValueType>&        vector_x,
    ValueType*     __restrict        vector_y_out)
{
    // Temporary storage for inter-thread fix-up after load-balanced work
    IDType     row_carry_out[256];     // The last row-id each worked on by each thread when it finished its path segment
    ValueType      value_carry_out[256];   // The running total within each thread when it finished its path segment

    #pragma omp parallel for schedule(static) num_threads(num_threads)
    for (int tid = 0; tid < num_threads; tid++)
    {
        // Merge list B (NZ indices)
        CountingInputIterator<IDType>  nonzero_indices(0);

        IDType num_merge_items     = a.get_dimensions()[0] + a.get_num_nnz();                          // Merge path total length
        IDType items_per_thread    = (num_merge_items + num_threads - 1) / num_threads;    // Merge items per thread

        // Find starting and ending MergePath coordinates (row-idx, nonzero-idx) for each thread
        int2    thread_coord;
        int2    thread_coord_end;
        int     start_diagonal      = std::min(items_per_thread * tid, num_merge_items);
        int     end_diagonal        = std::min(start_diagonal + items_per_thread, num_merge_items);

        MergePathSearch(start_diagonal, row_ptr, nonzero_indices, (int)a.get_dimensions()[0], (int)a.get_num_nnz(), thread_coord);
        MergePathSearch(end_diagonal, row_ptr, nonzero_indices, (int)a.get_dimensions()[0], (int)a.get_num_nnz(), thread_coord_end);

        // Consume whole rows
        thread_coord.x++;
        for (; thread_coord.x < thread_coord_end.x+1; ++thread_coord.x)
        {
            
            ValueType running_total = 0.0;
            for (; thread_coord.y < row_ptr[thread_coord.x]; ++thread_coord.y)
            {
                running_total += vals[thread_coord.y] * vector_x.get_vals()[cols[thread_coord.y]];
            }
            vector_y_out[(thread_coord.x)-1] = running_total;
        }

        // Consume partial portion of thread's last row
        ValueType running_total = 0.0;
        for (; thread_coord.y < thread_coord_end.y; ++thread_coord.y)
        {
            running_total += vals[thread_coord.y] * vector_x.get_vals()[cols[thread_coord.y]];
        }

        // Save carry-outs
        row_carry_out[tid] = thread_coord_end.x;
        value_carry_out[tid] = running_total;
    }

    // Carry-out fix-up (rows spanning multiple threads)
    for (int tid = 0; tid < num_threads - 1; ++tid)
    {
        if (row_carry_out[tid] < a.get_dimensions()[0])
            vector_y_out[row_carry_out[tid]] += value_carry_out[tid];
    }
}