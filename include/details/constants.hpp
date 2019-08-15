#pragma once
#include <cassert>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace cpptables {
namespace details {

template <typename SizeType> struct constants {};
template <> struct constants<std::uint32_t> {
	enum : std::uint32_t {
		k_null          = 0x7fffffff,
		k_invalid_bit   = 0x80000000,
		k_link_mask     = 0x7fffffff,
		k_spoiler_mask  = 0x7f000000,
		k_index_mask    = 0xff000000,
		k_spoiler_shift = 24
	};
};
template <> struct constants<std::uint64_t> {
	enum : std::uint64_t {
		k_null          = 0x7fffffffffffffff,
		k_invalid_bit   = 0x8000000000000000,
		k_link_mask     = 0x7fffffffffffffff,
		k_spoiler_mask  = 0x7f00000000000000,
		k_index_mask    = 0xff00000000000000,
		k_spoiler_shift = 56
	};
};

} // namespace details
} // namespace cpptables