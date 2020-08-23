
#pragma once
#include "constants.hpp"
#include "sparse_table_with_backref.hpp"

namespace cpptables {

namespace details {

template <typename Ty, typename Backref, typename SizeType> struct spt_backref {
	using plink = cpptables::link<Ty*, SizeType>;
	using link  = cpptables::link<Ty, SizeType>;

	template <typename T, typename SizeTy>
	static void set_link(T iObj, link iLink) {
		Backref::template set_link<std::remove_pointer_t<T>, SizeTy>(*iObj, iLink);
	}

	template <typename T, typename SizeTy> static link get_link(T iObj) {
		return Backref::template get_link<std::remove_pointer_t<T>, SizeTy>(*iObj);
	}

	template <typename T, typename SizeTy>
	static void set_link(T iObj, plink iLink) {
		Backref::template set_link<std::remove_pointer_t<T>, SizeTy>(*iObj, link((SizeType)iLink));
	}
};

template <typename Ty, typename SizeType> struct spt_storage {
	enum constants : SizeType {
		k_null        = 0,
		k_invalid_bit = 0x1,
		k_link_shift  = 1
	};

	spt_storage()                   = default;
	spt_storage(const spt_storage&) = default;
	spt_storage(spt_storage&&)      = default;

	spt_storage(Ty* iObject) : storage(iObject) {}

	void construct(Ty* iObject) { storage = iObject; }

	inline spt_storage& operator=(const spt_storage& iObject) = default;
	inline spt_storage& operator=(spt_storage&& iObject) = default;

	bool is_null() const {
		return (reinterpret_cast<std::uintptr_t>(storage) &
		        constants::k_invalid_bit) != 0;
	}

	void destroy() { storage = nullptr; }
	void set_next_free_index(SizeType iIndex) {

		(*reinterpret_cast<std::uintptr_t*>(&storage)) =
		    (iIndex << constants::k_link_shift) | constants::k_invalid_bit;
	}
	SizeType get_next_free_index() const {
		return static_cast<SizeType>(
		    (*reinterpret_cast<const std::uintptr_t*>(&storage)) >>
		    constants::k_link_shift);
	}
	const Ty* get() const { return storage; }
	Ty*& get() { return storage; }
	Ty* storage = nullptr;
};

template <typename Ty, typename SizeType, typename Allocator, typename Backref>
class sparse_table_of_pointers
    : public details::sparse_table_with_backref<
          Ty*, SizeType, Allocator, details::spt_backref<Ty, Backref, SizeType>,
          details::spt_storage<Ty, SizeType>> {
public:
	using base_type = details::sparse_table_with_backref<
          Ty*, SizeType, Allocator, details::spt_backref<Ty, Backref, SizeType>,
          details::spt_storage<Ty, SizeType>>;
	using ulink = cpptables::link<Ty, SizeType>;
	using link = typename base_type::link;
	inline void erase(ulink iIndex) { this->base_type::erase(link((SizeType)iIndex)); }
	inline const Ty& at(ulink iIndex) const {
		return *this->base_type::at(link((SizeType)iIndex));
	}
	inline Ty& at(ulink iIndex) { return *this->base_type::at(link((SizeType)iIndex)); }
};
} // namespace details
} // namespace cpptables
