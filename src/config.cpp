/*
 * config.cpp
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */

#include <iostream>
#include <fstream>
#include <exception>
#include "config.h"

#include "exception.h"
#include "log.h"


using namespace boost::program_options;

config::config()
{
  // TODO Auto-generated constructor stub

}

config::~config()
{
  // TODO Auto-generated destructor stub
}

bool config::init(int argc, char ** argv)
{
  // setup possible command line and config file options
  options_description cmd_line_desc("Command-Line Options");
  cmd_line_desc.add_options()
      ("help,h",                            "produce this message")
      ("config,c", value<std::string>(),    "configuration")
  ;
  options_description config_file_desc("Config File Options");
  config_file_desc.add_options()
      ("log.level",       value<std::string>(),   "log levels")
      ("log.destination", value<std::string>(),   "log destination: stderr, syslog, file name")
      ("log.time_format", value<std::string>(),   "log time format (see boost date/time docs)")
  ;

  // parse command line
  store(parse_command_line(argc, argv, cmd_line_desc), this->_data);
  notify(this->_data);

  // help?
  if (this->_data.count("help")) {
      std::cout << cmd_line_desc << std::endl;
      return false;
  }

  // config?
  if (this->_data.count("config")) {
      std::string config_file = this->_data["config"].as<std::string>();
      std::ifstream file(config_file.c_str(), std::ios_base::in);
      if(file.fail()) {
          throw exception("Unable to open file: " + config_file);
      }

      store(parse_config_file(file, config_file_desc), this->_data);
      log::write(log::LEVEL_INFO, "Loaded configuration file '%s'", config_file.c_str());
  }

  // initialize log file
  log::init(*this);

  // good - continue
  return true;
}
