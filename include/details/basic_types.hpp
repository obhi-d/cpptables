#include "constants.hpp"

namespace cpptables {

template <typename Ty, typename SizeType> struct link {
	using constants = details::constants<SizeType>;
	enum : SizeType { k_null = constants::k_null };

	link()              = default;
	link(const link& i) = default;
	link(SizeType i) : offset(i) {}
	inline operator SizeType() { return offset; }
	inline operator bool() { return offset != k_null; }

	SizeType offset = k_null;
};

namespace tags {
struct sparse_pointer {};
struct compact_indirect {};
struct sparse_direct {};

struct precise {};
struct fast {};

} // namespace tags

namespace details {
template <typename SizeType> struct index_t {
	using constants = details::constants<SizeType>;
	index_t()       = default;
	index_t(SizeType iID) : value(iID) {}
#ifdef L_DEBUG
	index_t(SizeType iIndex, std::uint8_t iSpoiler)
	    : index_val(iIndex | (static_cast<SizeType>(iSpoiler)
	                          << constants::k_spoiler_shift)) {}
	std::uint8_t spoiler() const {
		return static_cast<std::uint8_t>(index_val >> constants::k_spoiler_shift) &
		       0x7f;
	}
	SizeType index() const { return index_val & constants::k_index_mask; }
	SizeType value() const { return index_val; }
#else
	index_t(SizeType iIndex, std::uint8_t iSpoiler) : index_val(iIndex) {}
	constexpr std::uint8_t spoiler() const { return 0; }
	SizeType index() const { return index_val; }
	SizeType value() const { return index_val; }
#endif
	SizeType index_val;
};

} // namespace details

} // namespace cpptables