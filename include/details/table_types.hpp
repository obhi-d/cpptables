#pragma once
#include "basic_types.hpp"
#include "packed_table_with_indirection.hpp"
#include "sparse_table_of_pointers.hpp"
#include "sparse_table_with_backref.hpp"
#include "sparse_table_with_no_iter.hpp"
#include "sparse_table_with_sortedfree.hpp"
#include "sparse_table_with_validmap.hpp"
namespace cpptables {

/**!
 * Table interface common to all types of implementation
 */
template <unsigned TypeTagValue, typename Ty, auto BackrefMember = 0,
          typename SizeType  = std::uint32_t,
          typename Allocator = std::allocator<Ty>>
class table {};

constexpr auto tv_packed = tags_v<tags::packed>;

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tv_packed, Ty, BackrefMember, SizeType, Allocator>
    : public details::packed_table_with_indirection<Ty, SizeType, Allocator,
                                                    no_backref> {

public:
	enum : unsigned { tags = tv_packed };
};

template <typename Ty, typename Allocator = std::allocator<Ty>>
using tbl_packed = table<tv_packed, Ty, 0, std::uint32_t, Allocator>;

constexpr auto tv_packed_br = tags_v<tags::packed, tags::backref>;

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tv_packed_br, Ty, BackrefMember, SizeType, Allocator>
    : public details::packed_table_with_indirection<
          Ty, SizeType, Allocator, with_backref<BackrefMember>> {
public:
	enum : unsigned { tags = tv_packed_br };
};

template <typename Ty, auto BackrefMember,
          typename Allocator = std::allocator<Ty>>
using tbl_packed_br =
    table<tv_packed_br, Ty, BackrefMember, std::uint32_t, Allocator>;

constexpr auto tv_sparse_ptr = tags_v<tags::sparse, tags::pointer>;

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tv_sparse_ptr, Ty, BackrefMember, SizeType, Allocator>
    : public details::sparse_table_of_pointers<Ty, SizeType, Allocator,
                                               no_backref> {
public:
	enum : unsigned { tags = tv_sparse_ptr };
};

template <typename Ty, typename Allocator = std::allocator<Ty>>
using tbl_sparse_ptr = table<tv_sparse_ptr, Ty, 0, std::uint32_t, Allocator>;

constexpr auto tv_sparse_ptr_br =
    tags_v<tags::sparse, tags::pointer, tags::backref>;

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tv_sparse_ptr_br, Ty, BackrefMember, SizeType, Allocator>
    : public details::sparse_table_of_pointers<Ty, SizeType, Allocator,
                                               with_backref<BackrefMember>> {
public:
	enum : unsigned { tags = tv_sparse_ptr_br };
};

template <typename Ty, auto BackrefMember,
          typename Allocator = std::allocator<Ty>>
using tbl_sparse_ptr_br =
    table<tv_sparse_ptr_br, Ty, BackrefMember, std::uint32_t, Allocator>;

constexpr auto tv_sparse_br = tags_v<tags::sparse, tags::backref>;

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tv_sparse_br, Ty, BackrefMember, SizeType, Allocator>
    : public details::sparse_table_with_backref<Ty, SizeType, Allocator,
                                                with_backref<BackrefMember>> {
public:
	enum : unsigned { tags = tv_sparse_br };
};

template <typename Ty, auto BackrefMember,
          typename Allocator = std::allocator<Ty>>
using tbl_sparse_br =
    table<tv_sparse_br, Ty, BackrefMember, std::uint32_t, Allocator>;

constexpr auto tv_sparse_no_iter_br =
    tags_v<tags::sparse, tags::backref, tags::no_iter>;

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tv_sparse_no_iter_br, Ty, BackrefMember, SizeType, Allocator>
    : public details::sparse_table_with_no_iter<Ty, SizeType, Allocator,
                                                with_backref<BackrefMember>> {
public:
	enum : unsigned { tags = tv_sparse_no_iter_br };
};

template <typename Ty, auto BackrefMember,
          typename Allocator = std::allocator<Ty>>
using tbl_sparse_no_iter_br =
    table<tv_sparse_no_iter_br, Ty, BackrefMember, std::uint32_t, Allocator>;

constexpr auto tv_sparse_no_iter = tags_v<tags::sparse, tags::no_iter>;

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tv_sparse_no_iter, Ty, BackrefMember, SizeType, Allocator>
    : public details::sparse_table_with_no_iter<Ty, SizeType, Allocator,
                                                no_backref> {
public:
	enum : unsigned { tags = tv_sparse_no_iter };
};

template <typename Ty, typename Allocator = std::allocator<Ty>>
using tbl_sparse_no_iter =
    table<tv_sparse_no_iter, Ty, 0, std::uint32_t, Allocator>;

constexpr auto tv_sparse_sfree_br =
    tags_v<tags::sparse, tags::sortedfree, tags::backref>;

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tv_sparse_sfree_br, Ty, BackrefMember, SizeType, Allocator>
    : public details::sparse_table_with_sortedfree<
          Ty, SizeType, Allocator, with_backref<BackrefMember>> {
public:
	enum : unsigned { tags = tv_sparse_sfree_br };
};

template <typename Ty, auto BackrefMember,
          typename Allocator = std::allocator<Ty>>
using tbl_sparse_sfree_br =
    table<tv_sparse_sfree_br, Ty, BackrefMember, std::uint32_t, Allocator>;

constexpr auto tv_sparse_sfree = tags_v<tags::sparse, tags::sortedfree>;

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tv_sparse_sfree, Ty, BackrefMember, SizeType, Allocator>
    : public details::sparse_table_with_sortedfree<Ty, SizeType, Allocator,
                                                   no_backref> {
public:
	enum : unsigned { tags = tv_sparse_sfree };
};
template <typename Ty, typename Allocator = std::allocator<Ty>>
using tbl_sparse_sfree =
    table<tv_sparse_sfree, Ty, 0, std::uint32_t, Allocator>;

constexpr auto tv_sparse_vmap_br =
    tags_v<tags::sparse, tags::validmap, tags::backref>;

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tv_sparse_vmap_br, Ty, BackrefMember, SizeType, Allocator>
    : public details::sparse_table_with_validmap<Ty, SizeType, Allocator,
                                                 with_backref<BackrefMember>> {
public:
	enum : unsigned { tags = tv_sparse_vmap_br };
};

template <typename Ty, auto BackrefMember,
          typename Allocator = std::allocator<Ty>>
using tbl_sparse_vmap_br =
    table<tv_sparse_vmap_br, Ty, BackrefMember, std::uint32_t, Allocator>;

constexpr auto tv_sparse_vmap = tags_v<tags::sparse, tags::validmap>;

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tv_sparse_vmap, Ty, BackrefMember, SizeType, Allocator>
    : public details::sparse_table_with_validmap<Ty, SizeType, Allocator,
                                                 no_backref> {
public:
	enum : unsigned { tags = tv_sparse_vmap };
};

template <typename Ty, typename Allocator = std::allocator<Ty>>
using tbl_sparse_vmap = table<tv_sparse_vmap, Ty, 0, std::uint32_t, Allocator>;

} // namespace cpptables