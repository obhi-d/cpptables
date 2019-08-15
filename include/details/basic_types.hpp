#pragma once
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
struct compact {
	enum { value = 1 };
};
struct backref {
	enum { value = 2 };
};
struct sparse {
	enum { value = 4 };
};
struct pointer {
	enum { value = 8 };
};
struct no_iter {
	enum { value = 16 };
};
struct validmap {
	enum { value = 32 };
};
struct sortedfree {
	enum { value = 64 };
};

template <typename... Options> struct options {
	enum { value = (Options::value | ...) };
};
} // namespace tags

template <typename... Tags>
inline constexpr std::uint32_t tags_v = tags::options<Tags...>::value;

struct no_backref : std::false_type {
	template <typename Ty, typename SizeType>
	inline static void set_link(Ty& oObject, SizeType iIdx) {}
	template <typename Ty, typename SizeType>
	inline static SizeType get_link(Ty& iObject) {
		return SizeType();
	}
};
template <auto Member> struct with_backref : std::true_type {
	template <typename Ty, typename SizeType>
	inline static void set_link(Ty& oObject, SizeType iIdx) {
		*reinterpret_cast<SizeType*>(&(oObject.*Member)) = iIdx;
	}
	template <typename Ty, typename SizeType>
	inline static SizeType get_link(Ty& iObject) {
		return *reinterpret_cast<const SizeType*>(&(oObject.*Member));
	}
};

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