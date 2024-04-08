#ifndef QUADCONTAINER_H
#define QUADCONTAINER_H

#include "common.hpp"
#include <vector>

// specialized structure, since a vector was too slow. I want to be able to memcpy lots of data, not build/emplace one by one (compiler usually odes it when I don't want to)
// it is meant to be very unsafe, minimal and fast. no constructors for the objects are used, and they are never empty-initialized
template <typename T>
class QuadContainer {
public:
	QuadContainer() = delete; // for max performance you have to decide on an initial size > 0
	
	// !!!!!cap > 0 otherwise when doubling size it gets stuck at 0
	// didnt know a better way to check this at the time
	QuadContainer(std::size_t cap)
	: _sp(0), _capacity(cap)
	{
		_data = reinterpret_cast<T *>(std::malloc(sizeof(T) * cap));
	}

	~QuadContainer() {
		free(_data);
	}

	// copies data from the vector into here, reserving more space if needed
	constexpr void add(std::vector<T> &vec) {
		std::size_t og_sp = _sp;
		_sp += vec.size();
		if (og_sp == _sp) {
			return;
		}
		while (_sp > _capacity) {
			grow();
		}
		// how to force this into memcpy aligned???
		std::memcpy(reinterpret_cast<void *>(_data + og_sp), reinterpret_cast<void *>(vec.data()), sizeof(T) * (_sp - og_sp));
		// std::copy(vec.begin(), vec.end(), _data + og_sp); // like 1 fps worse (because I dont have LTO?????)
	}

	void clear() {
		_sp = 0;
	}

	// cursed
	template <typename... Args>
	void emplace_back(Args&&... args) {
		if (_sp == _capacity) {
			grow();
		}
		_data[_sp] = T(std::forward<Args>(args)...);
		_sp ++;
	}

	constexpr T &operator[](std::size_t index) const {
        return _data[index];
    }

	constexpr T &operator[](std::size_t index) {
        return _data[index];
    }

	constexpr T * data() const { return _data; } // const?????
	constexpr std::size_t size() const { return _sp; }
	constexpr std::size_t capacity() const { return _capacity; }
private:
	std::size_t _sp; // stack pointer (num elements)
	std::size_t _capacity; // capacity of underlying data structure. for max speed, starts at 

	T * _data;

	void grow() {
		_capacity *= 2;
		_data = reinterpret_cast<T *>(std::realloc(reinterpret_cast<void *>(_data), sizeof(T) * _capacity));
	}
};

#endif
