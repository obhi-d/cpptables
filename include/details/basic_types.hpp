#pragma once
#include "constants.hpp"
#include <type_traits>
#include <compare>
#include <concepts>

namespace cpptables {

template <typename Ty, typename SizeType> struct link {
	using constants = details::constants<SizeType>;
	enum : SizeType { k_null = constants::k_null };

	link()              = default;
	link(const link& i) = default;
  explicit link(SizeType i) : offset(i) {}

	template <typename Uy>
  explicit link(const link<Uy, SizeType>& i,
	     std::enable_if_t<std::is_convertible_v<Uy*, Ty*>>* = nullptr)
	    : offset(i.offset) {}

	link& operator=(const link& i) = default;

	template <typename Uy>
	link& operator=(
	    const link<Uy, SizeType>& i) requires std::convertible_to<Uy*, Ty*> {
		offset = i.offset;
		return *this;
	}

	inline explicit operator SizeType() const { return offset; }
	inline explicit operator bool() const { return offset != k_null; }
	inline auto operator <=> (link const& iSecond) const = default;

  inline friend auto operator <=> (SizeType iFirst, link const& iSecond) {
		return iFirst <=> iSecond.offset;
	}
  inline friend auto operator <=> (link const& iSecond, SizeType iFirst) {
    return iFirst <=> iSecond.offset;
  }
	SizeType offset = k_null;
};

namespace tags {
struct packed {
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

} // namespace tags

namespace details {

template <typename... Options> constexpr unsigned options() {
	return (Options::value | ...);
};

} // namespace details
template <typename... Tags>
inline constexpr unsigned tags_v = details::options<Tags...>();

struct no_backref : std::false_type {
	template <typename Ty, typename SizeType>
	inline static void set_link(Ty& oObject, link<Ty, SizeType> iIdx) {}
	template <typename Ty, typename SizeType>
	[[maybe_unused]] inline static link<Ty, SizeType> get_link(Ty const& iObject) {
		return {};
	}
};
template <auto Member> struct with_backref : std::true_type {
	template <typename Ty, typename SizeType>
	inline static void set_link(Ty& oObject, link<Ty, SizeType> iIdx) {
		*reinterpret_cast<SizeType*>(&(oObject.*Member)) = (SizeType)iIdx;
	}
	template <typename Ty, typename SizeType>
	[[maybe_unused]] inline static link<Ty, SizeType> get_link(Ty const& iObject) {
		return link<Ty, SizeType>(*reinterpret_cast<SizeType const*>(&(iObject.*Member)));
	}
};

namespace details {

template <typename U, typename V>
constexpr bool is_not_same_v = !std::is_same_v<U, V>;

template <typename T>
constexpr bool has_backref_v =
    !std::is_same_v<no_backref, T> && !std::is_same_v<std::false_type, T>;

template <typename SizeType> struct index_t {
	using constants = details::constants<SizeType>;
	index_t()       = default;
	index_t(SizeType iID) : val_(iID) {}
#ifdef CPPTABLES_DEBUG
	index_t(SizeType iIndex, std::uint8_t iSpoiler)
	    : val_(iIndex |
	           (static_cast<SizeType>(iSpoiler) << constants::k_spoiler_shift)) {}
	[[nodiscard]] std::uint8_t spoiler() const {
		return static_cast<std::uint8_t>(val_ >> constants::k_spoiler_shift) & 0x7f;
	}
	SizeType index() const { return val_ & constants::k_index_mask; }
	SizeType value() const { return val_; }
#else
	index_t(SizeType iIndex, std::uint8_t iSpoiler) : val_(iIndex) {}
	constexpr std::uint8_t spoiler() const { return 0; }
	SizeType index() const { return val_; }
	SizeType value() const { return val_; }
#endif
	SizeType val_;
};

} // namespace details

} // namespace cpptables