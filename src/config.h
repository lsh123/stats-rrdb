/*
 * config.h
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <string>
#include <boost/program_options.hpp>

class config
{
public:
  config();
  virtual ~config();

  bool init(int argc, char ** argv);

  bool has(const std::string & name);

  template<typename T>
  T get(const std::string & name, const T & default_value = T()) const {
    if (!this->_data.count(name)) {
        return default_value;
    }
    return this->_data[name].as<T>();
  }


private:
  boost::program_options::variables_map _data;
}; //  class config

#endif /* CONFIG_H_ */
