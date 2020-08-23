#pragma once
#include "basic_types.hpp"
#include "podvector.hpp"
#include <vector>

namespace cpptables {
namespace details {

template <typename Ty, typename SizeType, typename Allocator, typename Backref>
class packed_table_with_indirection {
	using vector_t = std::conditional_t<std::is_trivially_copyable_v<Ty>,
	                                    podvector<Ty, Allocator, SizeType>,
	                                    std::vector<Ty, Allocator>>;

public:
	using value_type = Ty;
	using size_type  = SizeType;
	using this_type =
	    packed_table_with_indirection<Ty, SizeType, Allocator, Backref>;
	using link                   = cpptables::link<Ty, SizeType>;
	using constants              = details::constants<size_type>;
	using index_t                = details::index_t<size_type>;
	using iterator               = typename vector_t::iterator;
	using const_iterator         = typename vector_t::const_iterator;
	using reverse_iterator       = typename vector_t::reverse_iterator;
	using const_reverse_iterator = typename vector_t::const_reverse_iterator;

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
		return static_cast<size_type>(items.size());
	}
	/**! Total number of slots valid in the table */
	size_type capacity() const noexcept {
		return static_cast<size_type>(indirection.size());
	}
	/**! Total number of slots to effieiencyl do parallel iteration */
	size_type range() const noexcept { return size(); }
	/**! Insert an object */
	link insert(Ty const& iObject) noexcept {
		SizeType location = static_cast<SizeType>(items.size());
		items.push_back(iObject);
		return do_insert(location);
	}
	/**! Emplace an object */
	template <typename... Args> link emplace(Args&&... iArgs) noexcept {
		SizeType location = static_cast<SizeType>(items.size());
		items.emplace_back(std::forward<Args>(iArgs)...);
		return do_insert(location);
	}
	/**! Erase an object */
	void erase(link iIndex) {
		SizeType id = iIndex;
#ifdef CPPTABLES_DEBUG
		index_t index(id);
		id = index.index();
		assert(spoilers[id] == index.spoiler());
		spoilers[id] = (spoilers[id] + 1) & 0x7f;
#endif
		if (has_backref_v<Backref>) {
			SizeType end_l = get_link(items.back());
#ifdef CPPTABLES_DEBUG
			index_t end_index(end_l);
			end_l = end_index.index();
#endif
			items[indirection[id]] = std::move(items.back());
			items.pop_back();
			indirection[end_l] = indirection[id];

		} else {
			items[indirection[id]] = std::move(items.back());
			items.pop_back();
			size_type filler_id = static_cast<size_type>(items.size());
			assert(filler_id < indirection.size());
			if (indirection[filler_id] == filler_id) {
				indirection[filler_id] = indirection[id];
			} else {
				for (long end = static_cast<long>(indirection.size() - 1); end >= 0;
				     --end) {
					if (indirection[end] == filler_id) {
						indirection[end] = indirection[id];
						break;
					}
				}
			}
		}
		indirection[id]  = first_free_index | constants::k_invalid_bit;
		first_free_index = id;
	}

	/**! Erase an object */
	/*std::enable_if_t<has_backref_v<Backref>>void*/ void erase(
	    Ty const& iObject) {
		assert(has_backref_v<Backref> && "Not supported without backreference");
		erase(Backref::template get_link<value_type, size_type>(iObject));
	}
	/**! Locate an object using its link */
	inline Ty& at(link iIndex) {
		SizeType id = iIndex;
#ifdef CPPTABLES_DEBUG
		index_t index(id);
		id = index.index();
		assert(spoilers[id] == index.spoiler());
#endif
		return items[indirection[id]];
	}
	/**! Locate an object using its link */
	inline Ty const& at(link iIndex) const {
		return const_cast<Ty const&>(const_cast<this_type*>(this)->at(iIndex));
	}

	// Iterators
	iterator begin() { return items.begin(); }
	iterator end() { return items.end(); }
	const_iterator begin() const { return items.begin(); }
	const_iterator end() const { return items.end(); }
	reverse_iterator rbegin() { return items.rbegin(); }
	reverse_iterator rend() { return items.rend(); }
	const_reverse_iterator rbegin() const { return items.rbegin(); }
	const_reverse_iterator rend() const { return items.rend(); }

	static void set_link(Ty& ioObj, link iLink) {
		Backref::template set_link<Ty, SizeType>(ioObj, iLink);
	}

	static link get_link(Ty const& ioObj) {
		return Backref::template get_link<Ty, SizeType>(ioObj);
	}

	void clear() {
		items.clear();
		indirection.clear();
#ifdef CPPTABLES_DEBUG
		spoilers.clear();
#endif
		first_free_index = constants::k_null;
	}

private:
	inline link do_insert(SizeType iLoc) {
		SizeType index = first_free_index;
		if (index == constants::k_null) {
			index = static_cast<SizeType>(indirection.size());
			indirection.emplace_back(iLoc);
#ifdef CPPTABLES_DEBUG
			spoilers.emplace_back(0);
#endif
		} else {
			first_free_index   = indirection[index] & constants::k_link_mask;
			indirection[index] = iLoc;
		}
#ifdef CPPTABLES_DEBUG
		index = index_t(index, spoilers[index]).value();
#endif
		Backref::template set_link<Ty, SizeType>(items[iLoc], link(index));
		return link(index);
	}

	template <typename Lambda, typename Type>
	inline static void for_each(Type& iCont, Lambda&& iLambda) {
		SizeType begin = 0;
		SizeType end   = static_cast<SizeType>(iCont.items.size());
		while (begin < end) {
			std::forward<Lambda>(iLambda)(iCont.items[begin++]);
		}
	}
	template <typename Lambda, typename Type>
	inline static void for_each(Type& iCont, SizeType iBegin, SizeType iEnd,
	                            Lambda&& iLambda) {
		SizeType begin = iBegin;
		SizeType end   = iEnd;
		while (begin < end) {
			std::forward<Lambda>(iLambda)(iCont.items[begin++]);
		}
	}

	vector_t items;
	std::vector<size_type> indirection;
#ifdef CPPTABLES_DEBUG
	std::vector<std::uint8_t> spoilers;
#endif
	size_type first_free_index = constants::k_null;
};
} // namespace details
} // namespace cpptables