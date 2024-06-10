#ifndef BITMAP_H
#define BITMAP_H

#include "common.hpp"
#include <bit>

template<std::size_t size>
struct Bitmap {
	constexpr Bitmap();
	constexpr Bitmap(const GLuint number);
	constexpr Bitmap(const Bitmap<32>& other);
	~Bitmap();

	constexpr void clear();
	constexpr bool operator[](GLuint index) const;
	constexpr bool operator[](GLuint index);
	constexpr Bitmap<size> operator&(const Bitmap<size> &other) const;
	constexpr Bitmap<size> operator&(const GLuint other) const;
	constexpr Bitmap<size> operator<<(const GLuint number) const;
	constexpr Bitmap<size> operator>>(const GLuint number) const;
	constexpr Bitmap<size> operator~() const;
	constexpr void operator=(const Bitmap<size> &other);
	constexpr bool operator==(const Bitmap<size> &other);
	constexpr void operator&=(const GLuint number);
	constexpr void operator&=(const Bitmap<size> &other);
	constexpr void operator|=(const GLuint number);
	constexpr void operator|=(const Bitmap<size> &other);
	constexpr void operator<<=(const GLuint number);
	constexpr void operator>>=(const GLuint number);
	constexpr bool operator!=(const GLuint number) const;
	constexpr bool operator!=(const Bitmap<size> &other) const;
	constexpr void operator++(int);
	constexpr void setBit(GLuint index);
	constexpr void clearBit(GLuint index);
	constexpr GLuint value() const;
	constexpr bool allTrue() const;
	constexpr GLuint trailing_ones() const;
	constexpr GLuint trailing_zeroes() const;
	constexpr void setAllTrue();
	constexpr void clearFromTo(GLuint start, GLuint finish);
	constexpr void clearFromStartTo(GLuint finish);
	constexpr void setFromTo(GLuint start, GLuint finish);
	constexpr void setFromStartTo(GLuint finish);
	constexpr void print() const;
	constexpr void printInverted() const; // easier to read when thinking about array indices
};

template<>
struct Bitmap<32> {
	uint32_t data;

	constexpr Bitmap()
	: data(0) { }

	constexpr Bitmap(const GLuint number)
	: data(number) { }

	constexpr Bitmap(const Bitmap<32>& other)
    : data(other.data) { }

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

	constexpr Bitmap<32> operator&(const Bitmap<32> &other) const {
		Bitmap<32> res;
		res.data = data & other.data;
		return res;
	}

	constexpr Bitmap<32> operator&(const GLuint other) const {
		Bitmap<32> res;
		res.data = data & other;
		return res;
	}

	constexpr Bitmap<32> operator<<(const GLuint number) const {
		Bitmap<32> res;
		res.data = data << number;
		return res;
	}

	constexpr Bitmap<32> operator>>(const GLuint number) const {
		Bitmap<32> res;
		res.data = data >> number;
		return res;
	}

	constexpr Bitmap<32> operator~() const {
		Bitmap<32> res;
		res.data = ~data;
		return res;
	}

	constexpr void operator=(const Bitmap<32> &other) {
		data = other.data;
	}

	constexpr bool operator==(const Bitmap<32> &other) {
		return data == other.data;
	}

	constexpr void operator&=(const GLuint number) {
		data &= number;
	}

	constexpr void operator&=(const Bitmap<32> &other) {
		data &= other.data;
	}

	constexpr void operator|=(const GLuint number) {
		data |= number;
	}

	constexpr void operator|=(const Bitmap<32> &other) {
		data |= other.data;
	}

	constexpr void operator<<=(const GLuint number) {
		data <<= number;
	}

	constexpr void operator>>=(const GLuint number) {
		data >>= number;
	}

	constexpr bool operator!=(const GLuint number) const {
		return data != number;
	}

	constexpr bool operator!=(const Bitmap<32> &other) const {
		return data != other.data;
	}

	constexpr void operator++(int) {
		data++;
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


	constexpr GLuint trailing_ones() const {
		return std::countr_one(data);
	}

	constexpr GLuint trailing_zeroes() const {
		return std::countr_zero(data);
	}

	constexpr void setAllTrue() {
		data = 0xFFFFFFFF;
	}

	constexpr void clearFromTo(GLuint start, GLuint finish) {
		uint32_t mask = ((0x1 << (finish - start + 1)) - 1) << start;
		data &= ~mask;
	}

	constexpr void clearFromStartTo(GLuint finish) {
		uint32_t mask = (0x1 << (finish + 1)) - 1;
		data &= ~mask;
	}

	constexpr void setFromTo(GLuint start, GLuint finish) {
		uint32_t mask = ((0x1 << (finish - start + 1)) - 1) << start;
		data &= mask;
	}

	constexpr void setFromStartTo(GLuint finish) {
		uint32_t mask = (0x1 << (finish + 1)) - 1;
		data &= mask;
	}

	constexpr void print() const {
		printf("%032b\n", data);
	}

	constexpr void printInverted() const {
		uint32_t reversed = 0;
		for (GLuint i = 0; i < 32; i++) {
			reversed <<= 1;
			reversed |= ((data >> i) & 1);
		}
		printf("%032b\n", reversed);
	}
};

#endif
