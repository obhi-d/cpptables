#pragma once
#include "basic_types.hpp"
#include <vector>

namespace cpptables {
namespace details {

template <typename Ty, typename SizeType = std::uint32_t,
          typename Allocator = std::allocator<Ty>,
          typename Backref   = std::false_type,
          typename Storage   = std::false_type>
class sparse_table_with_sortedfree {

	union alignas(alignof(Ty)) data_block {
		Ty object;
		SizeType integer;
		std::uint8_t storage[size_of(Ty)];

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
			new (storage) Ty(std::move(data_block(std::forward<Args>(args)...)));
		}
		void destroy() { object.~Ty(); }
	};
	using dbpointer = data_block*;

public:
	using element_type = Ty;
	using size_type    = SizeType;
	using this_type =
	    sparse_table_with_sortedfree<Ty, SizeType, Allocator, Backref, Storage>;
	using link      = link<Ty, SizeType>;
	using constants = details::constants<SizeType>;
	using index_t   = details::index_t<SizeType>;

	template <typename Container> class iterator_wrapper {
	public:
		iterator_wrapper() = default;
		iterator_wrapper(Container& iCont, const std::uint32_t iIt = 0)
		    : container(iCont), base(iIt), free_id(iCont.get_first_free_slot()) {
			to_valid_free_slot();
		}
		iterator_wrapper(const iterator_wrapper&) = default;
		iterator_wrapper(iterator_wrapper&&)      = default;
		iterator_wrapper& operator=(const iterator_wrapper&) = default;
		iterator_wrapper& operator=(iterator_wrapper&&) = default;

		inline iterator_wrapper& operator++() {
			++base;
			forward_valid();
			return *this;
		}

		inline iterator_wrapper operator++(int) {
			iterator_wrapper r(*this);
			++base;
			forward_valid();
			return r;
		}

		inline iterator_wrapper& operator--() {
			--base;
			backward_valid();
			return *this;
		}

		inline iterator_wrapper operator--(int) {
			iterator_wrapper r(*this);
			--base;
			backward_valid();
			return r;
		}

		inline iterator_wrapper& operator-=(difference_type iN) {
			advance(-iN);
			return *this;
		}

		inline iterator_wrapper& operator+=(difference_type iN) {
			advance(iN);
			return *this;
		}

		inline bool operator==(const iterator_wrapper& iOther) const {
			return base == iOther.base;
		}

		inline bool operator!=(const iterator_wrapper& iOther) const {
			return base != iOther.base;
		}

		friend inline difference_type operator-(const iterator_wrapper& iFirst,
		                                        const iterator_wrapper& iSecond) {
			return iFirst.base - iSecond.base;
		}

		friend inline difference_type distance(const iterator_wrapper& iFirst,
		                                       const iterator_wrapper& iSecond) {
			return iSecond - iFirst;
		}

		void advance(difference_type iN) {
			base += iN;
			if (iN < 0)
				backward_valid();
			else
				forward_valid();
		}

		auto operator*() { return container[base]; }
		auto operator-> () { return &container[base]; }

	private:
		inline void to_valid_free_slot() {
			while (free_id < base)
				free_id = container.get_next_free_slot(free_id);
		}

		bool is_valid() const {
			return base == container.range() || base != free_id;
		}
		void forward_valid() {
			while (!is_valid()) {
				++base;
				free_id = container.get_next_free_slot(free_id);
			}
		}
		void backward_valid() {
			do {
				free_id = container.get_first_free_slot();
				to_valid_free_slot();
			} while (!is_valid() && (--base > 0));
		}
		void forward_valid(difference_type iAmount) {
			while (iAmount--)
				forward_valid();
		}
		void backward_valid(difference_type iAmount) {
			while (iAmount--)
				backward_valid();
		}

		Container& container = nullptr;
		size_type base       = 0;
		size_type free_id    = constants::k_null;
	};

	using iterator               = iterator_wrapper<this_type>;
	using const_iterator         = iterator_wrapper<const this_type>;
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using value_type             = Ty;
	using allocator_type         = Allocator;
	using difference_type        = std::ptrdiff_t;
	using reference              = value_type&;
	using const_reference        = const value_type&;
	using pointer                = Ty*;
	using const_pointer          = const Ty*;

	~sparse_table_with_sortedfree() { destroy_and_deallocate(); }
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

	/**!
	 * Lambda called for each element, Lambda should accept Ty& parameter
	 */
	template <typename Lambda> void for_each(Lambda&& iLambda) {
		this_type::for_each(*this, std::forward<Lambda>(iLambda));
	}
	/**!
	 * Lambda called for each element, Lambda should accept Ty& parameter
	 */
	template <typename Lambda> void for_each(Lambda&& iLambda) const {
		this_type::for_each(*this, std::forward<Lambda>(iLambda));
	}
	/**!
	 * Lambda called for each element, Lambda should accept Ty& parameter
	 */
	template <typename Lambda>
	void for_each(size_type iBeg, size_type iEnd, Lambda&& iLambda) {
		this_type::for_each(*this, iBeg, iEnd, std::forward<Lambda>(iLambda));
	}
	/**!
	 * Lambda called for each element, Lambda should accept Ty& parameter
	 */
	template <typename Lambda>
	void for_each(size_type iBeg, size_type iEnd, Lambda&& iLambda) const {
		this_type::for_each(*this, iBeg, iEnd, std::forward<Lambda>(iLambda));
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
		return index;
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
		insert_free_index(id);
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

	// Iterators
	iterator begin() { return iterator(*this); }
	iterator end() { return iterator(*this, size_); }
	const_iterator begin() const { return iterator(*this); }
	const_iterator end() const { return iterator(*this, size_); }
	reverse_iterator rbegin() { return reverse_iterator(iterator(*this, size_)); }
	reverse_iterator rend() { return reverse_iterator(iterator(*this, 0)); }
	const_reverse_iterator rbegin() const {
		return const_reverse_iterator(const_iterator(*this, size_));
	}
	const_reverse_iterator rend() const {
		return const_reverse_iterator(const_iterator(*this, 0));
	}

	static void set_link(Ty& ioObj, size_type iLink) {}
	static size_type get_link(const Ty& ioObj) { return size_type(); }

	void clear() {
		size_        = 0;
		valid_count_ = 0;
#ifdef CPPTABLES_DEBUG
		spoilers.clear();
#endif
		first_free_index_ = constants::k_null;
	}
	inline size_type get_first_free_slot() const { return first_free_index_; }
	inline size_type get_next_free_slot(size_type iIdx) const {
		return items_[iIdx].get_integer();
	}

private:
	static void insert_free_index(size_type iItem) {
		size_type* prev = &first_free_index_;
		size_type curr  = first_free_index_;

		while (curr < iItem)
			curr = get_next_free_slot(curr);
		*prev = iItem;
		items_[iItem].set_integer(curr);
		return;
	}
	void push_back(const Ty& x) {
		if (capacity_ < size_ + 1)
			unchecked_reserve(size_ + std::max<size_type>(size_ >> 1, 1));
		data_[size_++].construct(x);
	}

	template <class... Args> void emplace_back(Args&&... args) {
		if (capacity_ < size_ + 1)
			unchecked_reserve(size_ + std::max<size_type>(size_ >> 1, 1));
		data_[size_++].construct(std::forward<Args>(args)...);
	}

	template <typename Lambda, typename Type>
	inline static void for_each(Type& iCont, Lambda&& iLambda) {
		size_type begin = 0;
		size_type end   = size_;
		size_type fri   = first_free_index_;
		for (; begin < end; ++begin) {
			if (begin != fri)
				std::forward<Lambda>(iLambda)(iCont.items_[begin].get());
			else
				fri = get_next_free_slot(fri);
		}
	}
	template <typename Lambda, typename Type>
	inline static void for_each(Type& iCont, size_type iBegin, size_type iEnd,
	                            Lambda&& iLambda) {
		size_type begin = iBegin;
		size_type end   = iEnd;
		size_type fri   = first_free_index_;
		while (fri < begin)
			fri = get_next_free_slot(fri);
		for (; begin < end; ++begin) {
			if (begin != fri)
				std::forward<Lambda>(iLambda)(iCont.items_[begin].get());
			else
				fri = get_next_free_slot(fri);
		}
	}
	inline dbpointer allocate(size_type n) {
		return reinterpret_cast<dppointer>(Allocator::allocate(n));
	}
	inline void deallocate() { Allocator::deallocate(items_, capacity); }

	inline void destroy_and_deallocate() {

		if constexpr (!std::is_trivially_destructible_v<Ty>) {
			size_type fri = first_free_index_;
			for (size_type i = 0; i < size_; ++i) {
				if (begin != fri)
					items_[i].get().~Ty();
				else
					fri = get_next_free_slot(fri);
			}
		}
		deallocate();
		capacity_    = 0;
		size_        = 0;
		valid_count_ = 0;
		usage_.clear();
	}

	inline void unchecked_reserve(size_type n) {
		dppointer d = allocate(n);
		if (std::is_trivially_copyable_v<Ty>)
			std::memcpy(d, items_, size_ * size_of(Ty));
		else {
			size_type mcopy = std::min<size_type>(size_, n);
			size_type fri   = first_free_index_;
			for (size_type i = 0; i < mcopy; ++i) {
				if (begin != fri) {
					d[i].construct(std::move(items_[i].get()));
					if constexpr (!std::is_trivially_destructible_v<Ty>) {
						items_[i].get().~Ty();
					}
				} else {
					fri = get_next_free_slot(fri);
					d[i].set_integer(items_[i].get_integer());
				}
			}
		}
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