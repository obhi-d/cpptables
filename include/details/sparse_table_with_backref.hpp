#pragma once
#include "storage_with_backref.hpp"
#include <vector>

namespace cpptables {
namespace details {

template <typename Ty, typename SizeType, typename Allocator, typename Backref,
          typename Storage = storage_with_backref<Ty, Backref, SizeType>>
class sparse_table_with_backref {

public:
	using size_type = SizeType;
	using this_type =
	    sparse_table_with_backref<Ty, SizeType, Allocator, Backref, Storage>;
	using link      = link<Ty, SizeType>;
	using constants = details::constants<size_type>;
	using index_t   = details::index_t<size_type>;

protected:
	using storage = Storage;
	using rebind_alloc =
	    typename std::allocator_traits<Allocator>::template rebind_alloc<storage>;
	using vector_t                    = std::vector<storage, rebind_alloc>;
	using base_iterator               = typename vector_t::iterator;
	using base_const_iterator         = typename vector_t::const_iterator;
	using base_reverse_iterator       = typename vector_t::reverse_iterator;
	using base_const_reverse_iterator = typename vector_t::const_reverse_iterator;

public:
	// random
	template <typename Iterator> class iterator_wrapper {
	public:
		using difference_type =
		    typename std::iterator_traits<Iterator>::difference_type;
		iterator_wrapper()                        = default;
		iterator_wrapper(const iterator_wrapper&) = default;
		iterator_wrapper(iterator_wrapper&&)      = default;
		~iterator_wrapper()                       = default;

		iterator_wrapper(Iterator&& iBeg, Iterator&& iEnd)
		    : base(std::move(iBeg)), end(std::move(iEnd)) {
			forward_valid();
		}
		iterator_wrapper(const Iterator& iBeg, const Iterator& iEnd)
		    : base(iBeg), end(iEnd) {
			forward_valid();
		}

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

		auto operator*() { return (*base); }
		auto operator-> () { return &(*base); }

	private:
		bool is_valid() const { return base == end || !(*base).is_null(); }
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

		Iterator base;
		Iterator end;
	};

public:
	using iterator               = iterator_wrapper<base_iterator>;
	using const_iterator         = iterator_wrapper<base_const_iterator>;
	using reverse_iterator       = iterator_wrapper<base_reverse_iterator>;
	using const_reverse_iterator = iterator_wrapper<base_const_reverse_iterator>;

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
		return static_cast<size_type>(items_.size());
	}
	/**! Total number of slots to effieiencyl do parallel iteration */
	size_type range() const noexcept { return capacity(); }

	inline link insert(const Ty& iObject) {
		SizeType index = first_free_index_;
		if (index == constants::k_null) {
			index = static_cast<SizeType>(items_.size());
			items_.push_back(iObject);
#ifdef CPPTABLES_DEBUG
			spoilers_.emplace_back(0);
#endif
		} else {
			first_free_index_ = items_[index].get_next_free_index();
			items_[index].construct(iObject);
		}
		SizeType link_numbr = index;
#ifdef CPPTABLES_DEBUG
		link_numbr = index_t(index, spoilers_[index]).value();
#endif
		set_link(items_[index].get(), link_numbr);
		valid_count_++;
		return link_numbr;
	}

	template <typename... Args> inline link emplace(Args&&... args) {
		SizeType index = first_free_index_;
		if (index == constants::k_null) {
			index = static_cast<SizeType>(items_.size());
			items_.emplace_back(std::forward<Args>(args)...);
#ifdef CPPTABLES_DEBUG
			spoilers_.emplace_back(0);
#endif
		} else {
			first_free_index_ = items_[index].get_next_free_index();
			items_[index].construct(std::forward<Args>(args)...);
		}
		SizeType link_numbr = index;
#ifdef CPPTABLES_DEBUG
		link_numbr = index_t(index, spoilers_[index]).value();
#endif
		set_link(items_[index].get(), link_numbr);
		valid_count_++;
		return index;
	}

	inline std::enable_if_t<!std::is_same_v<Backref, no_backref> &&
	                        !std::is_same_v<Backref, std::false_type>>
	erase(const Ty& iObject) {
		erase(get_link(iObject));
	}

	inline void erase(link iIndex) {
		SizeType id = iIndex;
#ifdef CPPTABLES_DEBUG
		index_t index(id);
		id = index.index();
		assert(spoilers_[id] == index.spoiler());
		spoilers_[id] = (spoilers_[id] + 1) & 0x7f;
#endif
		items_[id].destroy();
		items_[id].set_next_free_index(first_free_index_);
		valid_count_--;
		first_free_index_ = id;
	}

	inline Ty& at(link iIndex) {
		SizeType id = iIndex;
#ifdef CPPTABLES_DEBUG
		index_t index(id);
		id = index.index();
		assert(spoilers_[id] == index.spoiler());
#endif
		return items_[id].get();
	}

	inline const Ty& at(link iIndex) const {
		return const_cast<const Ty&>(const_cast<this_type*>(this)->at(iIndex));
	}

	// Iterators
	iterator begin() { return iterator(items_.begin(), items_.end()); }
	iterator end() { return iterator(items_.end(), items_.end()); }
	const_iterator begin() const {
		return iterator(items_.begin(), items_.end());
	}
	const_iterator end() const { return iterator(items_.end(), items_.end()); }
	reverse_iterator rbegin() {
		return reverse_iterator(items_.rbegin(), items_.rend());
	}
	reverse_iterator rend() {
		return reverse_iterator(items_.rend(), items_.rend());
	}
	const_reverse_iterator rbegin() const {
		return const_reverse_iterator(items_.rbegin(), items_.rend());
	}
	const_reverse_iterator rend() const {
		return const_reverse_iterator(items_.rend(), items_.rend());
	}

	static void set_link(Ty& ioObj, SizeType iLink) {
		Backref::set_link<Ty, SizeType>(ioObj, iLink);
	}

	static SizeType get_link(const Ty& ioObj) {
		return Backref::get_link<Ty, SizeType>(ioObj);
	}

	void clear() {
		for_each([](Ty& oObj) { oObj.~Ty(); });
		items_.clear();
		valid_count_ = 0;
#ifdef CPPTABLES_DEBUG
		spoilers_.clear();
#endif
		first_free_index_ = constants::k_null;
	}

private:
	template <typename Lambda, typename Type>
	inline static void for_each(Type& iCont, Lambda&& iLambda) {
		SizeType begin = 0;
		SizeType end   = static_cast<SizeType>(iCont.items_.size());
		for (; begin < end; ++begin) {
			if (!iCont.items_[begin].is_null())
				std::forward<Lambda>(iLambda)(iCont.items_[begin].get());
		}
	}
	template <typename Lambda, typename Type>
	inline static void for_each(Type& iCont, SizeType iBegin, SizeType iEnd,
	                            Lambda&& iLambda) {
		SizeType begin = iBegin;
		SizeType end   = iEnd;
		for (; begin < end; ++begin) {
			if (!iCont.items_[begin].is_null())
				std::forward<Lambda>(iLambda)(iCont.items_[begin].get());
		}
	}

	vector_t items_;
#ifdef CPPTABLES_DEBUG
	std::vector<std::uint8_t> spoilers_;
#endif
	size_type first_free_index_ = constants::k_null;
	size_type valid_count_      = 0;
};

} // namespace details
} // namespace cpptables
