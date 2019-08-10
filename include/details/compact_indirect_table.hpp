
#include "table_type.hpp"
#include <vector>

namespace cpptables {

template <typename Ty, typename Accessor = Ty,
          typename SizeType  = std::uint32_t,
          typename Allocator = std::allocator<Ty>>
class table<tags::compact_indirect, Ty, Accessor, SizeType, Allocator> {
	using vector_t = std::vector<Ty, Allocator>;

public:
	using size_type = SizeType;
	using this_type =
	    table<tags::compact_indirect, Ty, Accessor, SizeType, Allocator>;
	using link      = link<Ty, SizeType>;
	using constants = details::constants<size_type>;
	using index_t   = details::index_t<size_type>;
	/**!
	 * Make a non-const table view of some type
	 */
	template <template <class> ViewType> ViewType<this_type> make_view() {
		return ViewType<this_type>(this);
	}
	/**!
	 * Make a const table view of some type
	 */
	template <template <class> ViewType>
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
	/**! Total number of objects stored in the table */
	size_type size() const noexcept {
		return static_cast<size_type>(items.size());
	}
	/**! Total number of slots valid in the table */
	size_type capacity() const noexcept {
		return static_cast<size_type>(indirection.size());
	}
	/**! Insert an object */
	link insert(const Ty& iObject) noexcept {
		SizeType location = static_cast<SizeType>(items.size());
		items.push_back(iObject);
		return do_insert(iObject);
	}
	/**! Emplace an object */
	template <typename... Args> link emplace(Args&&... iArgs) noexcept {
		SizeType location = static_cast<SizeType>(items.size());
		items.emplace_back(std::forward<Args>(iArgs), ...);
		return do_insert(iObject);
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
		items[indirection[id]] = std::move(items.back());
		items.pop_back();
		indirection[id]  = first_free_index | constants::k_invalid_bit;
		first_free_index = id;
	}
	/**! Erase an object */
	void erase(const Ty& iObject) { erase(Accessor::get_link(iObject)); }
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
	inline const Ty& at(link iIndex) const {
		return const_cast<const Ty&>(const_cast<this_type*>(this)->at(iIndex));
	}

	// Iterators
	vector_t::iterator begin() { return items.begin(); }
	vector_t::iterator end() { return items.end(); }
	vector_t::const_iterator begin() const { return items.begin(); }
	vector_t::const_iterator end() const { return items.end(); }
	vector_t::iterator rbegin() { return items.rbegin(); }
	vector_t::iterator rend() { return items.rend(); }
	vector_t::const_iterator rbegin() const { return items.rbegin(); }
	vector_t::const_iterator rend() const { return items.rend(); }

	template <typename AnyTag> vector_t::iterator begin(AnyTag) {
		return items.begin();
	}
	template <typename AnyTag> vector_t::iterator end(AnyTag) {
		return items.end();
	}
	template <typename AnyTag> vector_t::const_iterator begin(AnyTag) const {
		return items.begin();
	}
	template <typename AnyTag> vector_t::const_iterator end(AnyTag) const {
		return items.end();
	}

	static void set_link(Ty& ioObj, link iLink) {
		Accessor::set_link(ioObj, iLink);
	}

	static link get_link(Ty& ioObj) { return Accessor::get_link(iLink); }

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
			first_free_index   = indirection[index] & Constants::k_link_mask;
			indirection[index] = iLoc;
		}
#ifdef CPPTABLES_DEBUG
		index = index_t(index, spoilers[index]).value();
#endif
		Accessor::set_link(iObject, index);
		return index;
	}

	template <typename Lambda, typename Type>
	inline static void for_each(Type& iCont, Lambda&& iLambda) {
		SizeType begin = 0;
		SizeType end   = static_cast<SizeType>(iCont.items.size());
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
} // namespace cpptables
