
#include <array>
#include <cassert>
#include <cpptables.hpp>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>

struct Object {
	using link            = cpptables::link<Object, std::uint32_t>;
	Object()              = default;
	Object(const Object&) = default;
	Object(Object&&)      = default;

	Object& operator=(const Object&) = default;
	Object& operator=(Object&&) = default;

	Object(std::string_view iName) : name(iName) {
		std::cout << std::hex << this << " " << name << " created.\n";
	}
	std::string name;
	std::uint32_t index = 0;

	~Object() {
		if (name.length() > 0)
			std::cout << std::hex << this << " " << name << " destroyed.\n";
	}

	static void set_link(Object& iInst, link iLink) { iInst.index = iLink; }
	static link get_link(const Object& iInst) { return iInst.index; }
};

std::array<std::string, 9> views = {"First",   "Second", "Third",
                                    "Fourth",  "Fifth",  "Sixth",
                                    "Seventh", "Eigth",  "Ninth"};

template <typename Cont> void insert(Cont& iCont) {
	for (auto& v : views)
		iCont.insert(Object(v + ".insert"));
}

template <typename Cont> void emplace(Cont& iCont) {
	for (auto& v : views)
		iCont.emplace(v + ".emplace");
}

template <typename Cont> void print(const Cont& iCont) {
	for (auto& val : iCont)
		std::cout << val.name;
}

int main() {
	cpptables::table<cpptables::tags_v<cpptables::tags::compact>, Object>
	    compact_indirect;
	cpptables::table<
	    cpptables::tags_v<cpptables::tags::compact, cpptables::tags::backref>,
	    Object, &Object::index>
	    sparse_direct;

	emplace(compact_indirect);
	compact_indirect.clear();
	emplace(sparse_direct);
	sparse_direct.clear();

	insert(sparse_direct);
	sparse_direct.clear();
	insert(compact_indirect);
	compact_indirect.clear();
}
