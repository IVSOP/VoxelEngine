#ifndef BITMAP_H
#define BITMAP_H

#include "common.hpp"
#include <bit>

template<GLuint size>


// switch to a std::bitset wrapper????????????????????????
struct Bitmap {
	uint8_t data[size / 8];

	Bitmap() = default; // data is not zeroed out at the start!!!!
	~Bitmap() = default;

	constexpr void clear() {
		for (GLuint i = 0; i < size / 8; i++) {
			data[i] = 0;
		}
	}

	constexpr bool operator[](GLuint index) const {
        return (data[index / 8] >> (index % 8)) & 0x1;
    }

	constexpr bool operator[](GLuint index) {
        return (data[index / 8] >> (index % 8)) & 0x1;
    }

	// got a brain fart. how do you do this in a single function?????
	constexpr void setTrue(GLuint index) {
		const uint8_t original = data[index / 8];
		const uint8_t value_byte = 0x00 | (1 << (index % 8));
		data[index / 8] = original | value_byte;
	}

	constexpr void setFalse(GLuint index) {
		const uint8_t original = data[index / 8];
		const uint8_t value_byte = 0xFF & (0 << (index % 8));
		data[index / 8] = original & value_byte;
	}

	constexpr GLuint value() const;
	constexpr bool allTrue() const;
	constexpr GLuint findNext() const; // pos of the first bit set to 1
	constexpr void print() const;
};

// Specialization for size 32
template<>
constexpr void Bitmap<32>::clear() {
	GLuint *ptr = reinterpret_cast<GLuint *>(&data[0]);
	*ptr = 0x00000000;
}
template<>
constexpr GLuint Bitmap<32>::value() const {
	const GLuint *ptr = reinterpret_cast<const GLuint *>(&data[0]);
	return *ptr;
}
template<>
constexpr bool Bitmap<32>::allTrue() const {
	return value() == 0xFFFFFFFF;
}
template<>
constexpr GLuint Bitmap<32>::findNext() const {
	// printf("find next called in %032b. returning %u\n", value(), std::countl_one(value()));
	return std::countr_one(value()); // countl?????? or r????? what a mess
}
template<>
constexpr void Bitmap<32>::print() const {
	// printf("value is %#08x\n", value());
	// printf("value is %#032b\n", value());
	printf("value is %032b\n", value());
}


#endif
