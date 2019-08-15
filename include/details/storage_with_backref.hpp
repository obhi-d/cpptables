#pragma once
#include "basic_types.hpp"

namespace cpptables {

namespace details {

template <typename Ty, typename Backref, typename SizeType>
struct alignas(alignof(Ty)) storage_with_backref {
	using constants = details::constants<SizeType>;

	union data_block {
		Ty object;
		std::uint8_t storage[sizeof(Ty)];

		inline bool is_null() const noexcept {
			return (static_cast<SizeType>(Backref::get_link<Ty, SizeType>(object)) &
			        constants::k_invalid_bit) != 0;
		}

		inline SizeType get_data() const noexcept {
			return static_cast<SizeType>(Backref::get_link<Ty, SizeType>(object));
		}

		inline void set_data(SizeType iData) noexcept {
			return Backref::set_link<Ty, SizeType>(object, iData);
		}

		data_block() noexcept {}
		data_block(const data_block& iOther) noexcept {
			if (!iOther.is_null())
				new (&object) Ty(iOther);
			else
				set_data(iOther.get_data());
		}
		data_block(data_block&& iOther) noexcept {
			if (!iOther.is_null())
				new (&object) Ty(std::move(iOther.object));
			else
				set_data(iOther.get_data());
		}
		data_block& operator=(const data_block& iOther) noexcept {
			if (!is_null()) {
				if (!iOther.is_null())
					object = iOther.object;
				else {
					object.~Ty();
					set_data(iOther.get_data());
				}
			} else {
				if (!iOther.is_null())
					new (&object) Ty(iOther.object);
				else
					set_data(iOther.get_data());
			}
			return *this;
		}
		data_block& operator=(data_block&& iOther) noexcept {
			if (!is_null()) {
				if (!iOther.is_null())
					object = std::move(iOther.object);
				else {
					object.~Ty();
					set_data(iOther.get_data());
				}
			} else {
				if (!iOther.is_null())
					new (&object) Ty(std::move(iOther.object));
				else
					set_data(iOther.get_data());
			}
			return *this;
		}

		data_block(const Ty& iObject) noexcept : object(iObject) {}
		data_block(Ty&& iObject) noexcept : object(std::move(iObject)) {}
		template <typename... Args>
		data_block(Args&&... iArgs) noexcept
		    : object(std::forward<Args>(args)...) {}
		~data_block() noexcept {}
	};

	storage_with_backref()                                    = default;
	storage_with_backref(const storage_with_backref& iObject) = default;
	storage_with_backref(storage_with_backref&& iObject)      = default;

	storage_with_backref(const Ty& iObject) : data(iObject) {}
	storage_with_backref(Ty&& iObject) : data(std::move(iObject)) {}

	~storage_with_backref() = default;
	template <typename... Args>
	storage_with_backref(Args&&... args) : data(std::forward<Args>(args)...) {}

	void construct(const Ty& iObject) { data = iObject; }
	void construct(Ty&& iObject) { data = std::move(iObject); }
	template <typename... Args> void construct(Args&&... args) {
		data = std::move(data_block(std::forward<Args>(args)...));
	}

	inline storage_with_backref& operator=(const storage_with_backref& iObject) =
	    default;

	inline storage_with_backref& operator=(storage_with_backref&& iObject) =
	    default;

	bool is_null() const { return data.is_null(); }
	void destroy() { data.object.~Ty(); }
	void set_next_free_index(SizeType iIndex) {
		assert((iIndex & constants::k_invalid_bit) == 0);
		data.set_data(iIndex | constants::k_invalid_bit);
	}
	SizeType get_next_free_index() const {
		return data.get_data() & constants::k_link_mask;
	}
	const Ty& get() const { return data.object; }
	Ty& get() { return data.object; }

	data_block data;
};
} // namespace details
} // namespace cpptables
