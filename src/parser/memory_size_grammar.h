/*
 * memory_size_grammar.h
 *
 *  Created on: Jun 22, 2013
 *      Author: aleksey
 */

#ifndef MEMORY_SIZE_GRAMMAR_H_
#define MEMORY_SIZE_GRAMMAR_H_

#include "grammar.h"
#include "memory_size.h"
#include "common/exception.h"

#include "memory_size.h"

template<typename Iterator>
class memory_size_grammar:
    public qi::grammar < Iterator, my::memory_size_t(), ascii::space_type >
{
  typedef qi::grammar < Iterator, my::memory_size_t(), ascii::space_type > base_type;

private:
  qi::rule < Iterator, my::memory_size_t(), ascii::space_type >  _start;

public:
  memory_size_grammar():
    base_type(_start)
  {
    _start =
          qi::uint_             [ qi::_val = qi::_1 ]
          >>
          (
             nocaselit("byte")  [ qi::_val *= MEMORY_SIZE_BYTE ]
           | nocaselit("bytes") [ qi::_val *= MEMORY_SIZE_BYTE ]
           | nocaselit("KB")    [ qi::_val *= MEMORY_SIZE_KILOBYTE ]
           | nocaselit("KBs")   [ qi::_val *= MEMORY_SIZE_KILOBYTE ]
           | nocaselit("MB")    [ qi::_val *= MEMORY_SIZE_MEGABYTE ]
           | nocaselit("MBs")   [ qi::_val *= MEMORY_SIZE_MEGABYTE ]
           | nocaselit("GB")    [ qi::_val *= MEMORY_SIZE_GIGABYTE ]
           | nocaselit("GBs")   [ qi::_val *= MEMORY_SIZE_GIGABYTE ]
           | qi::eps            [ qi::_val *= MEMORY_SIZE_BYTE ]
          )
     ;

    qi::on_error<qi::fail> (
        _start,
        grammar_error_handler("Error parsing memory size", qi::_1, qi::_2, qi::_3, qi::_4)
    );
  }
}; // memory_size_grammar

#endif /* MEMORY_SIZE_GRAMMAR_H_ */
