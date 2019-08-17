#pragma once
#include "basic_view.hpp"
#include "constants.hpp"
#include <functional>

namespace cpptables {
template <typename Container> class sorted_view : public basic_view<Container> {
public:
	using super        = basic_view<Container>;
	using size_type    = typename Container::size_type;
	using link         = typename Container::link;
	using value_type = typename Container::value_type;
	sorted_view(Container& iTy, const podvector<size_type>& iList)
	    : super(iTy, iList) {}
	sorted_view(Container& iTy, podvector<size_type>&& iList)
	    : super(iTy, std::move(iList)) {}
	sorted_view(Container& iTy) : super(iTy) {}
	sorted_view(sorted_view&& iOther) : super(std::move<super>(iOther)) {}
	sorted_view(const sorted_view& iOther) : super(iOther) {}

	inline sorted_view& operator=(sorted_view&& iOther) {
		super::operator=(std::move(iOther));
		return *this;
	}

	inline sorted_view& operator=(const sorted_view& iOther) {
		super::operator=(iOther);
		return *this;
	}
	inline void insert(link iCompIndex) {
		insert_sorted(items, (size_type)iCompIndex);
	}
	inline bool erase(link iCompIndex) {
		auto first = std::begin(items);
		auto last  = std::end(items);
		auto it    = std::lower_bound(first, last, (size_type)iCompIndex);
		if (it != last) {
			items.erase(it);
			return true;
		}
		return false;
	}
	inline size_type find(link iCompIndex) const {
		auto first = std::begin(items);
		auto last  = std::end(items);
		auto it    = std::lower_bound(first, last, (size_type)iCompIndex);
		if (it != last)
			return static_cast<size_type>(std::distance(first, it));
		return details::constants<size_type>::k_null;
	}

protected:
	static typename podvector<size_type>::iterator insert_sorted(
	    podvector<size_type>& iVec, size_type iItem) {
		return iVec.insert(std::upper_bound(iVec.begin(), iVec.end(), iItem),
		                   iItem);
	}
};
} // namespace cpptables
