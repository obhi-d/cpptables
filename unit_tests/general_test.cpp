
#include <array>
#include <cassert>
#include <cpptables.hpp>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>

struct Object {
	using link            = cpptables::link<Object, std::uint32_t>;
	Object()              = default;
	Object(const Object&) = default;
	Object(Object&&)      = default;

	Object& operator=(const Object&) = default;
	Object& operator=(Object&&) = default;

	Object(std::string_view iName) : name(iName) {}
	std::string name;
	std::uint32_t index = 0;

	~Object() {}

	struct LinkHash {
		std::size_t operator()(link l) const { return l.offset; }
	};

	static void set_link(Object& iInst, link iLink) { iInst.index = iLink; }
	static link get_link(const Object& iInst) { return iInst.index; }

	using fwset = std::unordered_map<link, std::string, Object::LinkHash>;
	using bwset = std::unordered_map<std::string, link>;
	using set   = std::pair<fwset, bwset>;
};

template <typename IntTy> IntTy range_rand(IntTy iBeg, IntTy iEnd) {
	return static_cast<IntTy>(
	    iBeg + (((double)rand() / (double)RAND_MAX) * (iEnd - iBeg)));
}

template <typename Cont>
void insert(Object::set& oSet, Cont& iCont, std::uint32_t iCount) {
	for (std::uint32_t i = 0; i < iCount; ++i) {
		std::string name  = std::to_string(i) + ".insert";
		auto l            = iCont.insert(Object(name));
		oSet.first[l]     = name;
		oSet.second[name] = l;
	}
}

template <typename Cont>
void emplace(Object::set& oSet, Cont& iCont, std::uint32_t iCount) {
	std::string name = v + ".emplace";
	for (auto& v : views) {
		std::string name  = std::to_string(i) + ".insert";
		auto l            = iCont.emplace(name);
		oSet.first[l]     = name;
		oSet.second[name] = l;
	}
}

template <typename Cont> void validate() {
	Cont cont;
	Object::set check;
	// Insert items
	std::uint32_t count = range_rand<std::uint32_t>(0, 100000);
	// insertion
	insert(check, cont, count);
	// for_each
	cont.for_each([&check](auto item) {
		assert(check.second.find(item.name) != check.second.end());
	});
	std::vector<Object::link> erase_list;
	cont.for_each(range_rand<std::uint32_t>(0, count >> 2),
	              range_rand<std::uint32_t>(count >> 2, count), [&](auto item) {
		              auto it = check.second.find(item.name);
		              assert(it != check.second.end());
		              erase_list.push_back((*it).second);
		              check.first.erase((*it).second);
		              check.second.erase(it);
	              });
	for (auto& i : erase_list) {
		cont.erase(i);
	}
	assert(cont.size() == check.first.size());
	cont.for_each([&check](auto item) {
		assert(check.second.find(item.name) != check.second.end());
	});
}

int main() {
	validate<
	    cpptables::table<cpptables::tags_v<cpptables::tags::compact>, Object>>();
	validate<cpptables::table<
	    cpptables::tags_v<cpptables::tags::compact, cpptables::tags::backref>,
	    Object, &Object::index>>();
	validate<cpptables::table<
	    cpptables::tags_v<cpptables::tags::sparse, cpptables::tags::backref>,
	    Object, &Object::index>>();
	validate<cpptables::table<
	    cpptables::tags_v<cpptables::tags::sparse, cpptables::tags::sortedfree,
	                      cpptables::tags::backref>,
	    Object, &Object::index>>();
	validate<cpptables::table<
	    cpptables::tags_v<cpptables::tags::sparse, cpptables::tags::validmap,
	                      cpptables::tags::backref>,
	    Object, &Object::index>>();
	validate<cpptables::table<
	    cpptables::tags_v<cpptables::tags::sparse, cpptables::tags::sortedfree>,
	    Object, &Object::index>>();
	validate<cpptables::table<
	    cpptables::tags_v<cpptables::tags::sparse, cpptables::tags::validmap>,
	    Object, &Object::index>>();
	return 0;
}
