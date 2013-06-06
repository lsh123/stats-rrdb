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
      // rrdb
      ("rrdb.path",                    value<std::string>(), "path to folder with RRDB data files (default: /var/lib/rrdb)")
      ("rrdb.default_policy",          value<std::string>(), "default metric policy (default: '1 sec for 1 min, 1 min for 1 year')")
      ("rrdb.flush_interval",          value<std::string>(), "how often we flus to disk (default: '10 sec')")

      // server_tcp
      ("server_tcp.address",           value<std::string>(), "tcp listener address (default: 0.0.0.0)")
      ("server_tcp.port",              value<int>(),         "tcp listener port (default: 9876)")
      ("server_tcp.max_message_size",  value<std::size_t>(), "tcp max message size in bytes (default: 4096)")
      ("server_tcp.thread_pool_size",  value<std::size_t>(), "tcp server number of worker threads (default: 10)")

      // server_udp
      ("server_udp.address",           value<std::string>(), "udp listener address (default: 0.0.0.0)")
      ("server_udp.port",              value<int>(),         "udp listener port (default: 9876)")
      ("server_udp.max_message_size",  value<std::size_t>(), "udp max message size in bytes (default: 2048)")
      ("server_udp.thread_pool_size",  value<std::size_t>(), "udp server number of worker threads (default: 5)")
      ("server_udp.send_response",     value<bool>(),        "udp server sends response (default: false)")

      // log
      ("log.level",       value<std::string>(),    "log level (default: info)")
      ("log.destination", value<std::string>(),    "log destination: stderr, syslog, file name (default: stderr)")
      ("log.time_format", value<std::string>(),    "log time format, see boost date/time docs: (default: %m/%d/%Y %H:%M:%S)")
  ;

  // parse command line
  store(parse_command_line(argc, argv, cmd_line_desc), _data);
  notify(_data);

  // help?
  if (_data.count("help")) {
      std::cout << cmd_line_desc << std::endl;
      std::cout << config_file_desc << std::endl;
      return false;
  }

  // config?
  if (_data.count("config")) {
      std::string config_file = _data["config"].as<std::string>();

      log::write(log::LEVEL_INFO, "Loading configuration file '%s'", config_file.c_str());
      std::ifstream file(config_file.c_str(), std::ios_base::in);
      if(file.fail()) {
          throw exception("Unable to open file '%s'", config_file.c_str());
      }

      store(parse_config_file(file, config_file_desc), _data);
      log::write(log::LEVEL_INFO, "Loaded configuration file '%s'", config_file.c_str());
  }

  // initialize log file
  log::init(*this);

  // good - continue
  return true;
}
