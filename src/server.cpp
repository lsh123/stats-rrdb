/*
 * server.cpp
 *
 *  Created on: Jun 2, 2013
 *      Author: aleksey
 */
#include <unistd.h>

#include <fstream>

#include "server.h"

#include "log.h"
#include "config.h"
#include "thread_pool.h"

#include "rrdb/rrdb.h"
#include "server_udp.h"
#include "server_tcp.h"

server::server(boost::shared_ptr<config> config) :
  _exit_signals(_io_service)

{
  log::write(log::LEVEL_DEBUG, "Starting the server");

  // init
  _rrdb.reset(new rrdb(config));
  _server_udp.reset(new server_udp(_io_service, _rrdb, config));
  _server_tcp.reset(new server_tcp(_io_service, _rrdb, config));

  // Register signal handlers so that the server may be shut down.
  _exit_signals.add(SIGHUP);
  _exit_signals.add(SIGINT);
  _exit_signals.add(SIGQUIT);
  _exit_signals.add(SIGABRT);
  _exit_signals.add(SIGTERM);
  _exit_signals.async_wait(boost::bind(
      &server::signal_handler,
      this,
      boost::asio::placeholders::error,
      boost::asio::placeholders::signal_number
  ));

  // done
  log::write(log::LEVEL_INFO, "Started the server");
}

server::~server()
{
}

void server::signal_handler(const boost::system::error_code& error, int signal_number)
{
  if (!error) {
      log::write(log::LEVEL_INFO, "Received signal %d, exiting", signal_number);
      this->stop();
      log::write(log::LEVEL_INFO, "Received signal %d, done", signal_number);
  }
}

void server::daemonize(const std::string & pid_file) 
{
  // Fork the process and have the parent exit. If the process was started
  // from a shell, this returns control to the user. Forking a new process is
  // also a prerequisite for the subsequent call to setsid().
  this->notify_before_fork();
  pid_t pid = fork();
  if(pid < 0) {
      log::write(log::LEVEL_CRITICAL, "Failed to fork the process: %d", errno);
      std::terminate();
  } else if(pid > 0) {
      log::write(log::LEVEL_DEBUG, "PID after the first fork is %d", pid);

      // We're in the parent process and need to exit but first let's notify
      // everyone
      this->notify_after_fork(true);
      exit(0);
  }
  this->notify_after_fork(false);

  // Make the process a new session leader. This detaches it from the
  // terminal.
  setsid();

  // A process inherits its working directory from its parent. This could be
  // on a mounted filesystem, which means that the running daemon would
  // prevent this filesystem from being unmounted. Changing to the root
  // directory avoids this problem.
  chdir("/");

  // The file mode creation mask is also inherited from the parent process.
  // We don't want to restrict the permissions on files created by the
  // daemon, so the mask is cleared.
  umask(0);

  // A second fork ensures the process cannot acquire a controlling terminal.
  this->notify_before_fork();
  pid = fork();
  if (pid < 0) {
      log::write(log::LEVEL_CRITICAL, "Failed to fork the process second time: %d", errno);
      std::terminate();
  } else if(pid > 0) {
      log::write(log::LEVEL_DEBUG, "PID after the second fork is %d", pid);

      // write out pid
      if(!pid_file.empty()) {
          std::fstream pidfs(pid_file.c_str(), std::ios_base::out);
          pidfs << pid;
      }

      // just exit, do nothing else but notify about exit
      this->notify_after_fork(true);
      exit(0);
  }
  this->notify_after_fork(false);

  // Close the standard streams. This decouples the daemon from the terminal
  // that started it.
  close(0); // STDIN
  close(1); // STDOUT
  close(2); // STDERR
}

void server::notify_before_fork()
{
  // Inform the io_service that we are about to become a daemon. The
  // io_service cleans up any internal resources, such as threads, that may
  // interfere with forking.
  _io_service.notify_fork(boost::asio::io_service::fork_prepare);
}

void server::notify_after_fork(bool is_parent)
{
  // Inform the io_service that we have finished becoming a daemon. The
  // io_service uses this opportunity to create any internal file descriptors
  // that need to be private to the new process
  if(is_parent) {
      _io_service.notify_fork(boost::asio::io_service::fork_parent);
  } else {
      _io_service.notify_fork(boost::asio::io_service::fork_child);
  }
}


void server::run()
{
  _rrdb->start();
  _server_udp->start();
  _server_tcp->start();

  _io_service.run();
}

void server::stop()
{
  _io_service.stop();
  _server_udp->stop();
  _server_tcp->stop();

  _rrdb->stop();
}

