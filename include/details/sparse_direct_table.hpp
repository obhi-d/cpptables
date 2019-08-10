
#include "table_type.hpp"
#include <vector>

namespace cpptables {

template <typename Ty, typename Accessor = Ty,
          typename SizeType  = std::uint32_t,
          typename Allocator = std::allocator<Ty>>
class table<tags::sparse_direct, Ty, Accessor, SizeType, Allocator> {

public:
	using size_type = SizeType;
	using this_type =
	    table<tags::sparse_direct, Ty, Accessor, SizeType, Allocator>;
	using link      = link<Ty, SizeType>;
	using constants = details::constants<size_type>;
	using index_t   = details::index_t<size_type>;

private:
	struct alignas(alignof(Ty)) place_holder {
		place_holder()                    = default;
		place_holder(const place_holder&) = default;
		place_holder(place_holder&&)      = default;

		place_holder(const Ty& iObject) { new (storage) Ty(iObject); }
		place_holder(Ty&& iObject) { new (storage) Ty(std::move(iObject)); }
		template <typename... Args> place_holder(Args&&... args) {
			new (storage) Ty(std::forward<Args>(args), ...);
		}

		void construct(const Ty& iObject) { new (storage) Ty(iObject); }
		void construct(Ty&& iObject) { new (storage) Ty(std::move(iObject)); }
		template <typename... Args> void construct(Args&&... args) {
			new (storage) Ty(std::forward<Args>(args), ...);
		}

		inline place_holder& operator=(const place_holder& iObject) = default;
		inline place_holder& operator=(place_holder&& iObject) = default;

		bool is_null() const {
			return (Accessor::get_link(*reinterpret_cast<const Ty*>(storage)) &
			        constants::k_invalid_bit) != 0;
		}

		void destroy() { reinterpret_cast<Ty*>(storage)->~Ty(); }
		void set_next_free_index(SizeType iIndex) {
			assert((iIndex & constants::k_invalid_bit) == 0);
			Accessor::set_link(*reinterpret_cast<Ty*>(storage),
			                   iIndex | constants::k_invalid_bit);
		}
		SizeType get_next_free_index() const {
			return Accessor::get_link(reinterpret_cast<const Ty*>(storage)) &
			       constants::k_link_mask;
		}
		const Ty& get() const { return *reinterpret_cast<const Ty*>(storage); }
		Ty& get() { return *reinterpret_cast<Ty*>(storage); }

		std::uint8_t storage[sizeof(Ty)];
	};

	using vector_t = std::vector<place_holder, Allocator>;

public:
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
	size_type size() const noexcept { return static_cast<size_type>(count); }
	/**! Total number of slots valid in the table */
	size_type capacity() const noexcept {
		return static_cast<size_type>(items.size());
	}

	inline link insert(const Ty& iObject) {
		SizeType index = first_free_index;
		if (index == constants::k_null) {
			index = static_cast<SizeType>(items.size());
			items.push_back(iObject);
#ifdef CPPTABLES_DEBUG
			spoilers.emplace_back(0);
#endif
		} else {
			first_free_index = items[index].get_next_free_index();
			items[index].construct(iObject);
		}
#ifdef CPPTABLES_DEBUG
		index = index_t(index, spoilers[index]).value();
#endif
		Accessor::set_link(iObject, index);
		return index;
	}

	template <typename... Args> inline link emplace(Args&&... args) {
		SizeType index = first_free_index;
		if (index == constants::k_null) {
			index = static_cast<SizeType>(items.size());
			items.emplace_back(std::forward<Args>(args), ...);
#ifdef CPPTABLES_DEBUG
			spoilers.emplace_back(0);
#endif
		} else {
			first_free_index = items[index].get_next_free_index();
			items[index].construct(std::forward<Args>(args), ...);
		}
#ifdef CPPTABLES_DEBUG
		index = index_t(index, spoilers[index]).Value();
#endif
		Accessor::set_link(iObject, index);
		return index;
	}

	inline void erase(const Ty& iObject) { erase(Accessor::get_link(iObject)); }

	inline void erase(link iIndex) {
		SizeType id = iIndex;
#ifdef CPPTABLES_DEBUG
		index_t index(id);
		id = index.index();
		assert(spoilers[id] == index.spoiler());
		spoilers[id] = (spoilers[id] + 1) & 0x7f;
#endif
		items[id].destroy();
		items[id].set_next_free_index(first_free_index);
		first_free_index = id;
	}

	inline Ty& at(link iIndex) {
		SizeType id = iIndex;
#ifdef CPPTABLES_DEBUG
		index_t index(id);
		id = index.Index();
		assert(spoilers[id] == index.Spoiler());
#endif
		return components[id].get();
	}

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
	template <typename Lambda, typename Type>
	inline static void for_each(Type& iCont, Lambda&& iLambda) {
		SizeType begin = 0;
		SizeType end   = static_cast<SizeType>(iCont.items.size());
		while (begin < end) {
			std::forward<Lambda>(iLambda)(iCont.items[begin++]);
		}
	}

	vector_t items;
#ifdef CPPTABLES_DEBUG
	std::vector<std::uint8_t> spoilers;
#endif
	size_type first_free_index = constants::k_null;
	size_type count            = 0;
};
} // namespace cpptables
