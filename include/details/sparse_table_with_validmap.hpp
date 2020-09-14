#pragma once
#include "basic_types.hpp"
#include <vector>

namespace cpptables {
namespace details {

template <typename Ty, typename SizeType = std::uint32_t,
          typename Allocator = std::allocator<Ty>,
          typename Backref   = std::false_type,
          typename Storage   = std::false_type>
class sparse_table_with_validmap : Allocator {

	union alignas(alignof(Ty)) data_block {
		Ty object;
		SizeType integer;
		std::uint8_t storage[sizeof(Ty)];

		inline SizeType get_integer() const noexcept { return integer; }
		inline void set_integer(SizeType iData) noexcept { integer = iData; }

		inline Ty const& get() const noexcept { return object; }
		inline Ty& get() noexcept { return object; }

		data_block() noexcept {}
		data_block(const data_block& iOther) = delete;
		data_block(data_block&& iOther)      = delete;
		data_block& operator=(const data_block& iOther) = delete;
		data_block& operator=(data_block&& iOther) = delete;

		data_block(Ty const& iObject) noexcept : object(iObject) {}
		data_block(Ty&& iObject) noexcept : object(std::move(iObject)) {}
		template <typename... Args>
		data_block(Args&&... iArgs) noexcept
		    : object(std::forward<Args>(iArgs)...) {}

		~data_block() noexcept {}

		void construct(Ty const& iObject) { new (storage) Ty(iObject); }
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
	    sparse_table_with_validmap<Ty, SizeType, Allocator, Backref, Storage>;
	using link            = cpptables::link<Ty, size_type>;
	using constants       = details::constants<SizeType>;
	using index_t         = details::index_t<SizeType>;
	using usage_map       = std::vector<std::uint32_t>;
	using allocator_type  = Allocator;
	using difference_type = std::ptrdiff_t;
	using reference       = value_type&;
	using const_reference = const value_type&;
	using pointer         = Ty*;
	using const_pointer   = const Ty*;

	static_assert(sizeof(size_type) <= sizeof(Ty),
	              "size_ of object should be greater than or equal to 4 bytes");

	template <typename Container> class iterator_wrapper {
	public:
		iterator_wrapper() = default;
		iterator_wrapper(Container& iCont, const std::uint32_t iIt = 0)
		    : container(iCont), base(iIt) {}
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
		bool is_valid() const {
			return base == container.range() || container.is_valid(base);
		}
		void forward_valid() {
			while (!is_valid())
				++base;
		}
		void backward_valid() {
			while (!is_valid())
				--base;
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
	};

	using iterator               = iterator_wrapper<this_type>;
	using const_iterator         = iterator_wrapper<const this_type>;
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	~sparse_table_with_validmap() { destroy_and_deallocate(); }
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

	template <bool iValue> constexpr void set_usage(size_type it) {
		size_type id = it >> 5;
		if (id >= usage_.size()) {
			if constexpr (iValue)
				return;
			usage_.resize(id + 1, 0);
		}
		std::uint32_t bit = static_cast<std::uint32_t>(it & 31);
		if constexpr (iValue)
			usage_[id] &= ~(1 << bit);
		else
			usage_[id] |= (1 << bit);
	}

	constexpr bool is_valid(size_type it) const {
		size_type id = it >> 5;
		return id >= usage_.size() ||
		       ((usage_[id] & (1 << static_cast<std::uint32_t>(it & 31))) == 0);
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

	inline link insert(Ty const& iObject) {
		size_type index = first_free_index_;
		if (index == constants::k_null) {
			index = static_cast<size_type>(size_);
			push_back(iObject);
#ifdef CPPTABLES_DEBUG
			spoilers.emplace_back(0);
#endif
		} else {
			first_free_index_ = items_[index].get_integer();
			if (first_free_index_ == constants::k_null)
				usage_.clear();
			items_[index].construct(iObject);
			set_usage<true>(index);
		}
		size_type link_numbr = index;
#ifdef CPPTABLES_DEBUG
		link_numbr = index_t(index, spoilers[index]).value();
#endif
		valid_count_++;
		return link(link_numbr);
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
			if (first_free_index_ == constants::k_null)
				usage_.clear();
			items_[index].construct(std::forward<Args>(args)...);
			set_usage<true>(index);
		}
		size_type link_numbr = index;
#ifdef CPPTABLES_DEBUG
		link_numbr = index_t(index, spoilers[index]).value();
#endif
		valid_count_++;
		return link(link_numbr);
	}

	inline void erase(link iIndex) {
		size_type id = iIndex.value();
#ifdef CPPTABLES_DEBUG
		index_t index(id);
		id = index.index();
		assert(spoilers[id] == index.spoiler());
		spoilers[id] = (spoilers[id] + 1) & 0x7f;
#endif
		items_[id].destroy();
		items_[id].set_integer(first_free_index_);
		valid_count_--;
		set_usage<false>(id);

		first_free_index_ = id;
	}

	inline Ty& at(link iIndex) {
		size_type id = iIndex.value();
#ifdef CPPTABLES_DEBUG
		index_t index(id);
		id = index.index();
		assert(spoilers[id] == index.spoiler());
#endif
		return items_[id].get();
	}

	inline Ty const& at(link iIndex) const {
		return const_cast<Ty const&>(const_cast<this_type*>(this)->at(iIndex));
	}

	inline Ty& at_index(size_type iIndex) { return items_[iIndex].get(); }

	inline Ty const& at_index(size_type iIndex) const {
		return const_cast<Ty const&>(
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

	static void set_link(Ty& ioObj, link iLink) {}
	static link get_link(Ty const& ioObj) { return {}; }

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
	void push_back(Ty const& x) {
		if (capacity_ < size_ + 1)
			unchecked_reserve(size_ + std::max<size_type>(size_ >> 1, 1));
		items_[size_++].construct(x);
	}

	template <class... Args> void emplace_back(Args&&... args) {
		if (capacity_ < size_ + 1)
			unchecked_reserve(size_ + std::max<size_type>(size_ >> 1, 1));
		items_[size_++].construct(std::forward<Args>(args)...);
	}

	template <typename Lambda, typename Type>
	inline static void for_each(Type& iCont, Lambda&& iLambda) {
		size_type begin = 0;
		size_type end   = iCont.size_;
		if (begin < iCont.usage_.size()) {
			for (; begin < end; ++begin) {
				if (iCont.is_valid(begin))
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
				if (iCont.is_valid(begin))
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

		if constexpr (!std::is_trivially_destructible_v<Ty>) {
			for (size_type i = 0; i < size_; ++i) {
				if (is_valid(i))
					items_[i].get().~Ty();
			}
		}
		deallocate();
		capacity_    = 0;
		size_        = 0;
		valid_count_ = 0;
		usage_.clear();
	}

	inline void unchecked_reserve(size_type n) {
		dbpointer d = allocate(n);
		if (std::is_trivially_copyable_v<Ty>)
			std::memcpy(d, items_, size_ * sizeof(Ty));
		else {
			size_type mcopy = std::min<size_type>(size_, n);
			for (size_type i = 0; i < mcopy; ++i) {
				if (is_valid(i)) {
					d[i].construct(std::move(items_[i].get()));
					if constexpr (!std::is_trivially_destructible_v<Ty>) {
						items_[i].get().~Ty();
					}
				} else {
					d[i].set_integer(items_[i].get_integer());
				}
			}
		}
		deallocate();
		items_    = d;
		capacity_ = n;
	}

	data_block* items_ = nullptr;
	usage_map usage_;
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