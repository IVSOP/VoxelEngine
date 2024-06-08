#ifndef BITMAP_H
#define BITMAP_H

#include "common.hpp"
#include <bit>

// template<GLuint size>


// // switch to a std::bitset wrapper????????????????????????
// struct Bitmap {
// 	uint8_t data[size / 8];

// 	Bitmap() = default; // data is not zeroed out at the start!!!!
// 	~Bitmap() = default;

// 	constexpr void clear() {
// 		for (GLuint i = 0; i < size / 8; i++) {
// 			data[i] = 0;
// 		}
// 	}

	// constexpr bool operator[](GLuint index) const {
    //     return (data[index / 8] >> (index % 8)) & 0x1;
    // }

	// constexpr bool operator[](GLuint index) {
    //     return (data[index / 8] >> (index % 8)) & 0x1;
    // }

// 	// got a brain fart. how do you do this in a single function?????
// 	constexpr void setTrue(GLuint index) {
// 		const uint8_t original = data[index / 8];
// 		const uint8_t value_byte = 0x00 | (1 << (index % 8));
// 		data[index / 8] = original | value_byte;
// 	}

// 	constexpr void setFalse(GLuint index) {
// 		const uint8_t original = data[index / 8];
// 		const uint8_t value_byte = 0xFF & (~ (1 << (index % 8))); // 0 << does not work properly for some reason, idk
// 		data[index / 8] = original & value_byte;
// 	}

//     // constexpr void setBit(std::size_t index) {
//     //     data[index / 8] |= (1 << (index % 8));
//     // }

//     // constexpr void clearBit(std::size_t index) {
//     //     data[index / 8] &= ~(1 << (index % 8));
//     // }

//     // constexpr bool checkBit(std::size_t index) const {
//     //     return data[index / 8] & (1 << (index % 8));
//     // }

// 	constexpr GLuint value() const;
// 	constexpr bool allTrue() const;
// 	constexpr GLuint findNext() const; // pos of the first bit set to 1
// 	constexpr GLuint findNextEmpty() const;
// 	constexpr void print() const;
// 	constexpr void setAllTrue();
// 	constexpr Bitmap<size> operator&(const Bitmap<size> &other) const;
// };

// // Specialization for size 32
// template<>
// constexpr Bitmap<32>::Bitmap() noexcept {
// 	GLuint *ptr = reinterpret_cast<GLuint *>(&data[0]);
// 	*ptr = 0x00000000;
// }

// template<>
// constexpr void Bitmap<32>::clear() {
// 	GLuint *ptr = reinterpret_cast<GLuint *>(&data[0]);
// 	*ptr = 0x00000000;
// }
// template<>
// constexpr GLuint Bitmap<32>::value() const {
// 	const GLuint *ptr = reinterpret_cast<const GLuint *>(&data[0]);
// 	return *ptr;
// }
// template<>
// constexpr bool Bitmap<32>::allTrue() const {
// 	return value() == 0xFFFFFFFF;
// }
// template<>
// constexpr GLuint Bitmap<32>::findNext() const {
// 	// printf("find next called in %032b. returning %u\n", value(), std::countl_one(value()))
// 	// test using __builtin_ctzll!!! has one less operation (or __builtin_ctz or __builtin_ctzl, whatever)
// 	return std::countr_one(value()); // countl?????? or r????? what a mess
// }
// template<>
// constexpr GLuint Bitmap<32>::findNextEmpty() const {
// 	return std::countr_zero(value());
// }
// template<>
// constexpr void Bitmap<32>::print() const {
// 	// printf("value is %#08x\n", value());
// 	// printf("value is %#032b\n", value());
// 	printf("value is %032b\n", value());
// }
// template<>
// constexpr void Bitmap<32>::setAllTrue() {
// 	GLuint *ptr = reinterpret_cast<GLuint *>(&data[0]);
// 	*ptr = 0xFFFFFFFF;
// }
// template<>
// constexpr Bitmap<32> Bitmap<32>::operator&(const Bitmap<32> &other) const {
// 	Bitmap<32> result;
	
// }

template<std::size_t size>
struct Bitmap {
	Bitmap();
	~Bitmap();

	constexpr void clear();
	constexpr bool operator[](GLuint index) const;
	constexpr bool operator[](GLuint index);
	constexpr Bitmap<size> operator&(const Bitmap<size> &other) const;
	constexpr void setBit(GLuint index);
	constexpr void clearBit(GLuint index);
	constexpr GLuint value() const;
	constexpr bool allTrue() const;
	constexpr GLuint findNext() const;
	constexpr GLuint findNextEmpty() const;
	constexpr void setAllTrue();
	constexpr void print() const;
};

template<>
struct Bitmap<32> {
	uint32_t data;

	Bitmap()
	: data(0) { }

	~Bitmap() = default;

	constexpr void clear() {
		data = 0;
	}

	constexpr bool operator[](GLuint index) const {
		return (data >> index) & 0x1;
	}

	constexpr bool operator[](GLuint index) {
		return (data >> index) & 0x1;
	}

	// constexpr Bitmap<32> operator&(const Bitmap<32> &other) const {
	// 	Bitmap<32> res;
	// 	res.data = data & other.data;
	// }

	constexpr void setBit(GLuint index) {
		data |= (0x1 << index);
	}

	constexpr void clearBit(GLuint index) {
		data &= ~(0x1 << index);
	}

	constexpr GLuint value() const {
		return data;
	}

	constexpr bool allTrue() const {
		return data == 0xFFFFFFFF;
	}

	constexpr GLuint findNext() const {
		return std::countr_one(data);
	}

	constexpr GLuint findNextEmpty() const {
		return std::countr_zero(data);
	}

	constexpr void setAllTrue() {
		data = 0xFFFFFFFF;
	}

	constexpr void print() const {
		printf("value is %032b\n", data);
	}
};

#endif
