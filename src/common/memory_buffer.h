/*
 * memory_buffer.h
 *
 *  Created on: Jun 13, 2013
 *      Author: aleksey
 */

#ifndef COMMON_MEMORY_BUFFER_H_
#define COMMON_MEMORY_BUFFER_H_

#include <vector>

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>

typedef std::vector<char>                                               t_memory_buffer_data;
typedef boost::iostreams::back_insert_device<t_memory_buffer_data>      t_memory_buffer_device;
typedef boost::iostreams::stream<t_memory_buffer_device>                t_memory_buffer;

#endif /* COMMON_MEMORY_BUFFER_H_ */
