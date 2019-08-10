
#include "basic_types.hpp"
namespace cpptables {

/**!
 * Table interface common to all types of implementation
 */
template <typename StorageTag, typename Ty, typename Accessor = Ty,
          typename SizeType  = std::uint32_t,
          typename Allocator = std::allocator<Ty>>
class table {
public:
	using size_type = SizeType;
	using this_type = table<StorageTag, Ty, Accessor, SizeType, Allocator>;
	using link      = link<Ty, SizeType>;
	/**!
	 * Make a non-const table view of some type
	 */
	template <template <class> ViewType> ViewType<this_type> make_view();
	/**!
	 * Make a const table view of some type
	 */
	template <template <class> ViewType>
	ViewType<const this_type> make_view() const;

	/**!
	 * Lambda called for each element, Lambda should accept Ty& parameter
	 */
	template <typename Lambda> void for_each(Lambda&& lambda);

	/**!
	 * Lambda called for each element, Lambda should accept const Ty& parameter
	 */
	template <typename Lambda> void for_each(Lambda&& lambda) const;

	/**! Total number of objects stored in the table */
	size_type size() const noexcept;
	/**! Total number of slots valid in the table */
	size_type capacity() const noexcept;
	/**! Insert an object */
	link insert(const Ty&) noexcept;
	/**! Emplace an object */
	template <typename... args> link emplace(args&&... args) noexcept;
	/**! Erase an object */
	void erase(link);
	/**! Erase an object */
	void erase(const Ty&);
	/**! Locate an object using its link */
	inline Ty& at(link);
	/**! Locate an object using its link */
	inline const Ty& at(link) const;
};

} // namespace cpptables