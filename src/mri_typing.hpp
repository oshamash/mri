#ifndef _MRI_TYPING_HPP_
#define _MRI_TYPING_HPP_

extern "C" {
	#include "mri_typing.h"
}

#include <map>
#include <array>
#include <vector>
#include <unordered_map>

template <typename K, typename V>
using mri_unordered_map = std::unordered_map<K, V>;

template <typename K, typename V>
using mri_ordered_map = std::map<K, V>;

template <typename T>
using mri_vector = std::vector<T>;

template <typename T, size_t N>
using mri_array = std::array<T, N>;

#endif
