#pragma once
#include "podvector.hpp"
#include <functional>

namespace cpptables {
template <typename Container> class basic_view {
public:
	using size_type    = typename Container::size_type;
	using link         = typename Container::link;
	using element_type = typename Container::element_type;

	basic_view(Container& iTy, const podvector<size_type>& iList)
	    : container(iTy), items(iList) {}
	basic_view(Container& iTy, podvector<size_type>&& iList)
	    : container(iTy), items(std::move(iList)) {}
	basic_view(Container& iTy) : container(iTy) {}
	basic_view(basic_view&& iOther)
	    : container(std::move(iOther.container)), items(std::move(iOther.items)) {
	}
	basic_view(const basic_view& iOther)
	    : container(iOther.container), items(iOther.items) {}

	inline basic_view& operator=(basic_view&& iOther) {
		container = iTy.container;
		items     = std::move(iOther.items);
		return *this;
	}

	inline basic_view& operator=(const basic_view& iOther) {
		container = iTy.container;
		items     = iOther.items;
		return *this;
	}
	inline size_type size() const { return static_cast<size_type>(items.size()); }
	template <typename Lambda> inline void for_each(Lambda&& iLambda) const {
		size_type end = size();
		for (size_type i = 0; i < end; ++i) {
			std::forward<Lambda>(iLambda)(container.get().at(items[i]));
		}
	}
	template <typename Lambda>
	inline void for_each(size_type iFirst, size_type iLirst,
	                     Lambda&& iLambda) const {
		for (; iFirst < iLast; ++iFirst) {
			std::forward<Lambda>(iLambda)(container.get()[items[iFirst]]);
		}
	}
	inline element_type& at(size_type iIndex) {
		return container.get()[items[iIndex]];
	}
	inline const element_type& at(size_type iIndex) const {
		return container.get()[items[iIndex]];
	}
	inline void insert(const element_type& iComp) {
		insert(container.get().get_link(iComp));
	}
	inline void insert(link iCompIndex) { items.push_back(iCompIndex); }
	inline void push_back(const element_type& iComp) {
		items.push_back(container.get().get_link(iComp));
	}
	inline void push_back(link iCompIndex) { items.push_back(iCompIndex); }
	inline bool erase(const element_type& iComp) {
		return erase(container.get().get_link(iComp));
	}
	inline bool erase(link iCompIndex) {
		items[find(iCompIndex)] = items.back();
		items.pop_back();
	}
	inline size_type find(const element_type& iComp) const {
		return find(container.get().get_link(iComp));
	}
	inline size_type find(link iCompIndex) const {
		auto first = std::begin(items);
		auto last  = std::end(items);
		auto it    = std::find(first, last, (size_type)iCompIndex);
		if (it != last)
			return static_cast<size_type>(std::distance(first, it));
		return details::constants<size_type>::k_null;
	}

protected:
	podvector<size_type> items;
	std::reference_wrapper<Container> container;
};
} // namespace cpptables
