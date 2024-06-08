#ifndef BITMAP_H
#define BITMAP_H

#include "common.hpp"
#include <bit>

template<std::size_t size>
struct Bitmap {
	Bitmap();
	~Bitmap();

	constexpr void clear();
	constexpr bool operator[](GLuint index) const;
	constexpr bool operator[](GLuint index);
	Bitmap<size> operator&(const Bitmap<size> &other) const;
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

	Bitmap<32> operator&(const Bitmap<32> &other) const {
		Bitmap<32> res;
		res.data = data & other.data;
		return res;
	}

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
