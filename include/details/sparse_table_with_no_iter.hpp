#pragma once
#include "basic_types.hpp"
#include <vector>

namespace cpptables {
namespace details {

template <typename Ty, typename SizeType = std::uint32_t,
          typename Allocator = std::allocator<Ty>,
          typename Backref   = std::false_type,
          typename Storage   = std::false_type>
class sparse_table_with_no_iter : Allocator {

	static_assert(
	    std::is_trivially_copyable_v<Ty> && std::is_trivially_destructible_v<Ty>,
	    "Type should be trivially copyable and trivially destructible!");
	union alignas(alignof(Ty)) data_block {
		Ty object;
		SizeType integer;
		std::uint8_t storage[sizeof(Ty)];

		inline SizeType get_integer() const noexcept { return integer; }
		inline void set_integer(SizeType iData) noexcept { integer = iData; }

		inline const Ty& get() const noexcept { return object; }
		inline Ty& get() noexcept { return object; }

		data_block() noexcept {}
		data_block(const data_block& iOther) = delete;
		data_block(data_block&& iOther)      = delete;
		data_block& operator=(const data_block& iOther) = delete;
		data_block& operator=(data_block&& iOther) = delete;

		data_block(const Ty& iObject) noexcept : object(iObject) {}
		data_block(Ty&& iObject) noexcept : object(std::move(iObject)) {}
		template <typename... Args>
		data_block(Args&&... iArgs) noexcept
		    : object(std::forward<Args>(args)...) {}

		~data_block() noexcept {}

		void construct(const Ty& iObject) { new (storage) Ty(iObject); }
		void construct(Ty&& iObject) { new (storage) Ty(std::move(iObject)); }
		template <typename... Args> void construct(Args&&... args) {
			new (storage) Ty(std::forward<Args>(args)...);
		}
		void destroy() { object.~Ty(); }
	};
	using dbpointer = data_block*;

public:
	using value_type = Ty;
	using size_type  = SizeType;
	using this_type =
	    sparse_table_with_no_iter<Ty, SizeType, Allocator, Backref, Storage>;
	using link            = link<Ty, SizeType>;
	using constants       = details::constants<SizeType>;
	using index_t         = details::index_t<SizeType>;
	using value_type      = Ty;
	using allocator_type  = Allocator;
	using difference_type = std::ptrdiff_t;
	using reference       = value_type&;
	using const_reference = const value_type&;
	using pointer         = Ty*;
	using const_pointer   = const Ty*;

	~sparse_table_with_no_iter() { destroy_and_deallocate(); }
	/**!
	 * Make a non-const table view of some type
	 */
	template <template <class> class ViewType> ViewType<this_type> make_view() {
		return ViewType<this_type>(this);
	}
	/**!
	 * Make a const table view of some type
	 */
	template <template <class> class ViewType>
	ViewType<const this_type> make_view() const {
		return ViewType<const this_type>(this);
	}

	/**! Total number of objects stored in the table */
	size_type size() const noexcept {
		return static_cast<size_type>(valid_count_);
	}
	/**! Total number of slots valid in the table */
	size_type capacity() const noexcept {
		return static_cast<size_type>(capacity_);
	}
	/**! Total number of slots to effieiencyl do parallel iteration */
	size_type range() const noexcept { return size_; }

	inline link insert(const Ty& iObject) {
		size_type index = first_free_index_;
		if (index == constants::k_null) {
			index = static_cast<size_type>(size_);
			push_back(iObject);
#ifdef CPPTABLES_DEBUG
			spoilers.emplace_back(0);
#endif
		} else {
			first_free_index_ = items_[index].get_integer();
			items_[index].construct(iObject);
		}
		size_type link_numbr = index;
#ifdef CPPTABLES_DEBUG
		link_numbr = index_t(index, spoilers[index]).value();
#endif
		set_link(items_[index].get(), link_numbr);
		return link_numbr;
	}

	template <typename... Args> inline link emplace(Args&&... args) {
		size_type index = first_free_index_;
		if (index == constants::k_null) {
			index = static_cast<size_type>(size_);
			emplace_back(std::forward<Args>(args)...);
#ifdef CPPTABLES_DEBUG
			spoilers.emplace_back(0);
#endif
		} else {
			first_free_index_ = items_[index].get_integer();
			items_[index].construct(std::forward<Args>(args)...);
		}
		size_type link_numbr = index;
#ifdef CPPTABLES_DEBUG
		link_numbr = index_t(index, spoilers[index]).value();
#endif
		set_link(items_[index].get(), link_numbr);
		return index;
	}

	inline /*std::enable_if_t<has_backref_v<Backref>>*/ void erase(
	    const Ty& iObject) {
		assert(has_backref_v<Backref> && "Not supported without backreference");
		erase(get_link(iObject));
	}

	inline void erase(link iIndex) {
		size_type id = iIndex;
#ifdef CPPTABLES_DEBUG
		index_t index(id);
		id = index.index();
		assert(spoilers[id] == index.spoiler());
		spoilers[id] = (spoilers[id] + 1) & 0x7f;
#endif
		items_[id].destroy();
		items_[id].set_integer(first_free_index_);
		first_free_index_ = id;
		valid_count_--;
	}

	inline Ty& at(link iIndex) {
		size_type id = iIndex;
#ifdef CPPTABLES_DEBUG
		index_t index(id);
		id = index.index();
		assert(spoilers[id] == index.spoiler());
#endif
		return items_[id].get();
	}

	inline const Ty& at(link iIndex) const {
		return const_cast<const Ty&>(const_cast<this_type*>(this)->at(iIndex));
	}

	inline Ty& at_index(size_type iIndex) { return items_[iIndex].get(); }

	inline const Ty& at_index(size_type iIndex) const {
		return const_cast<const Ty&>(
		    const_cast<this_type*>(this)->at_index(iIndex));
	}

	static void set_link(Ty& ioObj, size_type iLink) {
		if constexpr (has_backref_v<Backref>) {
			Backref::set_link(ioObj, iLink);
		}
	}
	static size_type get_link(const Ty& ioObj) {
		if constexpr (has_backref_v<Backref>) {
			return Backref::get_link(ioObj);
		}
		return size_type();
	}

	void clear() {
		usage_.clear();
		size_        = 0;
		valid_count_ = 0;
#ifdef CPPTABLES_DEBUG
		spoilers.clear();
#endif
		first_free_index_ = constants::k_null;
	}

private:
	void push_back(const Ty& x) {
		if (capacity_ < size_ + 1)
			unchecked_reserve(size_ + std::max<size_type>(size_ >> 1, 1));
		items_[size_++].construct(x);
		valid_count_++;
	}

	template <class... Args> void emplace_back(Args&&... args) {
		if (capacity_ < size_ + 1)
			unchecked_reserve(size_ + std::max<size_type>(size_ >> 1, 1));
		items_[size_++].construct(std::forward<Args>(args)...);
		valid_count_++;
	}

	template <typename Lambda, typename Type>
	inline static void for_each(Type& iCont, Lambda&& iLambda) {
		size_type begin = 0;
		size_type end   = iCont.size_;
		if (begin < iCont.usage_.size()) {
			for (; begin < end; ++begin) {
				if (!iCont.is_valid(begin))
					std::forward<Lambda>(iLambda)(iCont.items_[begin].get());
			}
		} else {
			for (; begin < end; ++begin) {
				std::forward<Lambda>(iLambda)(iCont.items_[begin].get());
			}
		}
	}
	template <typename Lambda, typename Type>
	inline static void for_each(Type& iCont, size_type iBegin, size_type iEnd,
	                            Lambda&& iLambda) {
		size_type begin = iBegin;
		size_type end   = iEnd;
		if (begin < iCont.usage_.size()) {
			for (; begin < end; ++begin) {
				if (!iCont.is_valid(begin))
					std::forward<Lambda>(iLambda)(iCont.items_[begin].get());
			}
		} else {
			for (; begin < end; ++begin) {
				std::forward<Lambda>(iLambda)(iCont.items_[begin].get());
			}
		}
	}
	inline dbpointer allocate(size_type n) {
		return reinterpret_cast<dbpointer>(Allocator::allocate(n));
	}
	inline void deallocate() {
		Allocator::deallocate(reinterpret_cast<Ty*>(items_), capacity_);
	}

	inline void destroy_and_deallocate() {
		deallocate();
		capacity_    = 0;
		size_        = 0;
		valid_count_ = 0;
	}

	inline void unchecked_reserve(size_type n) {
		dbpointer d = allocate(n);
		std::memcpy(d, items_, size_ * sizeof(Ty));
		deallocate();
		items_    = d;
		capacity_ = n;
	}

	data_block* items_          = nullptr;
	size_type size_             = 0;
	size_type capacity_         = 0;
	size_type valid_count_      = 0;
	size_type first_free_index_ = constants::k_null;
#ifdef CPPTABLES_DEBUG
	std::vector<std::uint8_t> spoilers;
#endif
};
} // namespace details
} // namespace cpptables