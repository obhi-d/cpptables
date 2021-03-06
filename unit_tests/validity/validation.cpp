#include <array>
#include <cassert>
#include <catch2/catch.hpp>
#include <cpptables.hpp>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>

struct SObject {
	using link          = cpptables::link<SObject, std::uint32_t>;
	char name[256]      = {};
	std::uint32_t index = 0;

	struct LinkHash {
		std::size_t operator()(link l) const { return l.offset; }
	};

	void set_name(std::string_view iName) {
		std::strncpy(name, iName.data(),
		             std::min<std::size_t>(iName.length(), 255));
	}
	static void set_link(SObject& iInst, link iLink) { iInst.index = (std::uint32_t)iLink; }
	static link get_link(const SObject& iInst) { return link(iInst.index); }

	inline SObject* operator->() { return this; }
	inline const SObject* operator->() const { return this; }
	using fwset = std::unordered_map<link, std::string, SObject::LinkHash>;
	using bwset = std::unordered_map<std::string, link>;
	using set   = std::pair<fwset, bwset>;
};

struct CObject {
	using link              = cpptables::link<CObject, std::uint32_t>;
	CObject()               = default;
	CObject(CObject const&) = default;
	CObject(CObject&&)      = default;

	CObject& operator=(CObject const&) = default;
	CObject& operator=(CObject&&) = default;

	CObject(std::string_view iName) : name(iName) {}

	inline CObject* operator->() { return this; }
	inline CObject const* operator->() const { return this; }
	void set_name(std::string_view iName) { name = iName; }
	std::string name    = "default";
	std::uint32_t index = 0;

	~CObject() {}

	struct LinkHash {
		std::size_t operator()(link l) const { return l.offset; }
	};

	static void set_link(CObject& iInst, link iLink) { iInst.index = (std::uint32_t)iLink; }
	static link get_link(CObject const& iInst) { return link(iInst.index); }

	using fwset = std::unordered_map<link, std::string, CObject::LinkHash>;
	using bwset = std::unordered_map<std::string, link>;
	using set   = std::pair<fwset, bwset>;
};

template <typename IntTy> IntTy range_rand(IntTy iBeg, IntTy iEnd) {
	return static_cast<IntTy>(
	    iBeg + (((double)rand() / (double)RAND_MAX) * (iEnd - iBeg)));
}

template <typename Cont> struct helper {
	static constexpr bool is_p = std::is_pointer_v<typename Cont::value_type>;
	using val_t                = typename Cont::value_type;

	using utype = std::conditional_t<is_p, std::remove_pointer_t<val_t>, val_t>;
	using set_t = typename utype::set;
	using link  = typename utype::link;
	using cleanup_list =
	    std::conditional_t<is_p, std::vector<std::unique_ptr<utype>>,
	                       std::size_t>;

	static void insert(set_t& oSet, std::uint32_t iOffset, Cont& iCont,
	                   std::uint32_t iCount, [[maybe_unused]] cleanup_list& oL) {
		for (std::uint32_t i = 0; i < iCount; ++i) {
			std::string name = std::to_string(i + iOffset) + ".o";
			link l;
			if constexpr (std::is_pointer_v<typename Cont::value_type>) {
				utype* data = new utype();
				oL.emplace_back(data);
				l = link((std::uint32_t)iCont.insert(data));
				iCont.at(l).set_name(name);
			} else {
				l = iCont.insert(typename Cont::value_type());
				iCont.at(l).set_name(name);
			}
			oSet.first[l]     = name;
			oSet.second[name] = l;
		}
	}

	static void emplace(set_t& oSet, std::uint32_t iOffset, Cont& iCont,
	                    std::uint32_t iCount, [[maybe_unused]] cleanup_list& oL) {
		for (std::uint32_t i = 0; i < iCount; ++i) {
			std::string name = std::to_string(i + iOffset) + ".o";
			link l;
			if constexpr (is_p) {
				utype* data = new utype();
				oL.emplace_back(data);
				l = link((std::uint32_t)iCont.emplace(data));
				iCont.at(l).set_name(name);
			} else {
				l = iCont.emplace();
				iCont.at(l).set_name(name);
			}
			oSet.first[l]     = name;
			oSet.second[name] = l;
		}
	}
};

template <typename Cont> void validate() {
	Cont cont;
	typename helper<Cont>::set_t check;
	typename helper<Cont>::cleanup_list cleaner;
	std::uint32_t last_offset = 0;
	for (int times = 0; times < 4; times++) {

		// Insert items
		std::uint32_t count = range_rand<std::uint32_t>(10, 1000);
		// insertion
		helper<Cont>::insert(check, last_offset + 0, cont, count >> 1, cleaner);
		REQUIRE(cont.size() == check.first.size());
		// emplace
		helper<Cont>::emplace(check, last_offset + (count >> 1), cont, count >> 1,
		                      cleaner);
		REQUIRE(cont.size() == check.first.size());
		last_offset += count;
		// for_each
		if constexpr ((Cont::tags & cpptables::tags::no_iter::value) != 0) {
			REQUIRE(cont.size() == check.first.size());
			REQUIRE(check.second.size() == check.first.size());
			std::uint32_t ec = range_rand<std::uint32_t>(1, count >> 2);
			for (std::uint32_t i = 0; i < ec; ++i) {
				std::string name =
				    std::to_string(range_rand<std::uint32_t>(0, count)) + ".o";
				auto it = check.second.find(name);
				if (it != check.second.end() &&
				    range_rand<std::uint32_t>(0, 100) > 50) {
					cont.erase(it->second);
					REQUIRE(check.first.find(it->second) != check.first.end());
					check.first.erase(it->second);
					check.second.erase(it);
				}
			}
			REQUIRE(cont.size() == check.first.size());
			REQUIRE(check.second.size() == check.first.size());
		} else {
			std::vector<typename helper<Cont>::link> erase_list;
			cont.for_each([&check](auto item) {
				REQUIRE(check.second.find(item->name) != check.second.end());
			});
			REQUIRE(cont.size() == check.first.size());
			REQUIRE(check.second.size() == check.first.size());
			cont.for_each(
			    range_rand<std::uint32_t>(0, count >> 2),
			    range_rand<std::uint32_t>(count >> 2, count), [&](auto item) {
				    auto it = check.second.find(item->name);
				    REQUIRE(it != check.second.end());
				    if (range_rand<std::uint32_t>(0, 100) > 50) {
					    erase_list.push_back((*it).second);
					    REQUIRE(check.first.find(it->second) != check.first.end());
					    check.first.erase(it->second);
					    check.second.erase(it);
				    }
			    });
			for (auto& i : erase_list) {
				cont.erase(i);
			}
			REQUIRE(cont.size() == check.first.size());
			REQUIRE(check.second.size() == check.first.size());
			cont.for_each([&check](auto item) {
				REQUIRE(check.second.find(item->name) != check.second.end());
			});
		}
	}
}

TEST_CASE("Validate tbl_packed", "[tbl_packed]") {
	validate<cpptables::tbl_packed<CObject>>();
	validate<cpptables::tbl_packed_br<CObject, &CObject::index>>();
}
TEST_CASE("Validate tbl_sparse_br", "[tbl_sparse_br]") {
	// msvc bug
	validate<cpptables::tbl_sparse_br<CObject, &CObject::index>>();
}
TEST_CASE("Validate tbl_sparse_sfree_br", "[tbl_sparse_sfree_br]") {
	validate<cpptables::tbl_sparse_sfree_br<CObject, &CObject::index>>();
}
TEST_CASE("Validate tbl_sparse_vmap_br", "[tbl_sparse_vmap_br]") {
	validate<cpptables::tbl_sparse_vmap_br<CObject, &CObject::index>>();
}
TEST_CASE("Validate tbl_sparse_sfree", "[tbl_sparse_sfree]") {
	validate<cpptables::tbl_sparse_sfree<CObject>>();
}
TEST_CASE("Validate tbl_sparse_vmap", "[tbl_sparse_vmap]") {
	validate<cpptables::tbl_sparse_vmap<CObject>>();
}
TEST_CASE("Validate tbl_sparse_no_iter", "[tbl_sparse_no_iter]") {
	validate<cpptables::tbl_sparse_no_iter<SObject>>();
}
TEST_CASE("Validate tbl_sparse_no_iter_br", "[tbl_sparse_no_iter_br]") {
	validate<cpptables::tbl_sparse_no_iter_br<SObject, &SObject::index>>();
}
TEST_CASE("Validate tbl_sparse_ptr", "[tbl_sparse_ptr]") {
	validate<cpptables::tbl_sparse_ptr<CObject>>();
}
TEST_CASE("Validate tbl_sparse_ptr_br", "[tbl_sparse_ptr_br]") {
	validate<cpptables::tbl_sparse_ptr_br<CObject, &CObject::index>>();
}
