#pragma once
#include "basic_types.hpp"

namespace cpptables {

namespace details {

template <typename Ty, typename Backref, typename SizeType>
struct alignas(alignof(Ty)) storage_with_backref {
	using constants = details::constants<SizeType>;

	storage_with_backref() noexcept { set_null(); }
	storage_with_backref(const storage_with_backref& iCopy) noexcept {
		if (!iCopy.is_null())
			new (&storage) Ty(iCopy.object());
		else
			set_link_index(iCopy.get_link_index());
	}
	storage_with_backref(storage_with_backref&& iMove) noexcept {
		if (!iMove.is_null())
			new (&storage) Ty(std::move(iMove.object()));
		else
			set_link_index(iMove.get_link_index());
	}
	storage_with_backref& operator=(const storage_with_backref& iOther) noexcept {
		if (!is_null()) {
			if (!iOther.is_null())
				object() = iOther.object();
			else {
				object().~Ty();
				set_link_index(iOther.get_link_index());
			}
		} else {
			if (!iOther.is_null())
				new (&storage) Ty(iOther.object());
			else
				set_link_index(iOther.get_link_index());
		}
		return *this;
	}
	storage_with_backref& operator=(storage_with_backref&& iOther) noexcept {
		if (!is_null()) {
			if (!iOther.is_null())
				object() = std::move(iOther.object());
			else {
				object().~Ty();
				set_link_index(iOther.get_link_index());
			}
		} else {
			if (!iOther.is_null())
				new (&storage) Ty(std::move(iOther.object()));
			else
				set_link_index(iOther.get_link_index());
		}
		return *this;
	}
	storage_with_backref(Ty const& iObject) noexcept {
		new (&storage) Ty(iObject);
	}
	storage_with_backref(Ty&& iObject) noexcept {
		new (&storage) Ty(std::move(iObject));
	}
	template <typename... Args> storage_with_backref(Args&&... args) {
		new (&storage) Ty(std::forward<Args>(args)...);
	}

	~storage_with_backref() {
		if (!is_null())
			destroy();
	}

	inline Ty& object() noexcept { return reinterpret_cast<Ty&>(storage); }
	inline Ty const& object() const noexcept {
		return reinterpret_cast<Ty const&>(storage);
	}

	inline SizeType get_link_index() const noexcept {
		return static_cast<SizeType>(
		    Backref::template get_link<Ty, SizeType>(object()));
	}
	inline void set_link_index(SizeType iData) noexcept {
		return Backref::template set_link<Ty, SizeType>(object(), iData);
	}
	inline bool is_null() const noexcept {
		return (static_cast<SizeType>(
		            Backref::template get_link<Ty, SizeType>(object())) &
		        constants::k_invalid_bit) != 0;
	}
	inline void set_null() noexcept {
		set_link_index(constants::k_invalid_bit);
	}

	void construct(Ty const& iObject) { new (&storage) Ty(iObject); }
	void construct(Ty&& iObject) { new (&storage) Ty(std::move(iObject)); }
	template <typename... Args> void construct(Args&&... args) {
		new (&storage) Ty(std::forward<Args>(args)...);
	}
	void destroy_if_not_null() {
		if (!is_null()) {
			destroy();
			set_null();
		}
	}
	void destroy() { object().~Ty(); }
	void set_next_free_index(SizeType iIndex) {
		assert((iIndex & constants::k_invalid_bit) == 0);
		set_link_index(iIndex | constants::k_invalid_bit);
	}
	SizeType get_next_free_index() const {
		return get_link_index() & constants::k_link_mask;
	}
	Ty const& get() const { return object(); }
	Ty& get() { return object(); }

	std::aligned_storage_t<sizeof(Ty), alignof(Ty)> storage;
};
} // namespace details
} // namespace cpptables
