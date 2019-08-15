#pragma once
#include "basic_types.hpp"
#include "compact_indirect_table.hpp"
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

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tags_v<tags::compact>, Ty, BackrefMember, SizeType, Allocator>
    : public details::compact_indirect_table<Ty, SizeType, Allocator,
                                             no_backref> {};

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tags_v<tags::compact, tags::backref>, Ty, BackrefMember, SizeType,
            Allocator>
    : public details::compact_indirect_table<Ty, SizeType, Allocator,
                                             with_backref<BackrefMember>> {};

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tags_v<tags::sparse, tags::pointer>, Ty, BackrefMember, SizeType,
            Allocator>
    : public details::sparse_table_of_pointers<Ty, SizeType, Allocator,
                                               no_backref> {};

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tags_v<tags::sparse, tags::pointer, tags::backref>, Ty,
            BackrefMember, SizeType, Allocator>
    : public details::sparse_table_of_pointers<Ty, SizeType, Allocator,
                                               with_backref<BackrefMember>> {};

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tags_v<tags::sparse, tags::backref>, Ty, BackrefMember, SizeType,
            Allocator>
    : public details::sparse_table_with_backref<Ty, SizeType, Allocator,
                                                with_backref<BackrefMember>> {};

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tags_v<tags::sparse, tags::no_iter, tags::backref>, Ty,
            BackrefMember, SizeType, Allocator>
    : public details::sparse_table_with_no_iter<Ty, SizeType, Allocator,
                                                with_backref<BackrefMember>> {};

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tags_v<tags::sparse, tags::no_iter>, Ty, BackrefMember, SizeType,
            Allocator>
    : public details::sparse_table_with_no_iter<Ty, SizeType, Allocator,
                                                no_backref> {};

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tags_v<tags::sparse, tags::sortedfree, tags::backref>, Ty,
            BackrefMember, SizeType, Allocator>
    : public details::sparse_table_with_sortedfree<
          Ty, SizeType, Allocator, with_backref<BackrefMember>> {};

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tags_v<tags::sparse, tags::sortedfree>, Ty, BackrefMember, SizeType,
            Allocator>
    : public details::sparse_table_with_sortedfree<Ty, SizeType, Allocator,
                                                   no_backref> {};

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tags_v<tags::sparse, tags::validmap, tags::backref>, Ty,
            BackrefMember, SizeType, Allocator>
    : public details::sparse_table_with_validmap<Ty, SizeType, Allocator,
                                                 with_backref<BackrefMember>> {
};

template <typename Ty, auto BackrefMember, typename SizeType,
          typename Allocator>
class table<tags_v<tags::sparse, tags::validmap>, Ty, BackrefMember, SizeType,
            Allocator>
    : public details::sparse_table_with_validmap<Ty, SizeType, Allocator,
                                                 no_backref> {};

} // namespace cpptables