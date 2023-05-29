#ifndef _MRI_GENERAL_HPP_
#define _MRI_GENERAL_HPP_

extern "C" {
	#include "mri_general.h"
}

template <typename T>
struct c_return {};

template <>
struct c_return<bool> {
	int	value;

	constexpr c_return(bool val)
	: value(val ? 0 : -1)
	{}
};

#endif
