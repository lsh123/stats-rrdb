/*
 * memory_size.h
 *
 *  Created on: Jun 6, 2013
 *      Author: aleksey
 */

#ifndef MEMORY_SIZE_H_
#define MEMORY_SIZE_H_

#include <string>

#include "common/types.h"

/**
 * Memory size string representation:
 *
 * <n>KB|MB|GB
 *
 */
my::memory_size_t memory_size_parse(const std::string & str);
std::string memory_size_write(const my::memory_size_t & memory_size);
std::string memory_size_get_name(const my::memory_size_t & unit, const my::memory_size_t & val);

#endif /* MEMORY_SIZE_H_ */
