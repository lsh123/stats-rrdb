/*
 * memory_buffer.h
 *
 *  Created on: Jun 13, 2013
 *      Author: aleksey
 */

#ifndef MEMORY_BUFFER_H_
#define MEMORY_BUFFER_H_

#include <vector>

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>

typedef std::vector<char>                                          memory_buffer_data_t;
typedef boost::iostreams::back_insert_device<memory_buffer_data_t> memory_buffer_device_t;
typedef boost::iostreams::stream<memory_buffer_device_t>           memory_buffer_t;

#endif /* MEMORY_BUFFER_H_ */
