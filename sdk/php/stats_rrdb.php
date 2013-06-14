<?php
/*
 * stts_rrdb.php
 *
 *  Created on: Jun 9, 2013
 *      Author: aleksey
 */

class StatsRRDB {
	const Mode_Udp = "udp";
	const Mode_Tcp = "tcp";

	private $_mode;
	private $_host;
	private $_port;
	private $_buf_size;
	
	public function __construct($mode = self::Mode_Udp, $host = '127.0.0.1', $port = 9876, $buf_size = 1024)
	{
		$this->_mode = $mode;
		$this->_host = $host;
		$this->_port = $port;
		$this->_buf_size = $buf_size;
	}

	public function send_command($command, $ignore_reply = false)
	{
		// open socket
		$fp = stream_socket_client("{$this->_mode}://{$this->_host}:{$this->_port}", $errno, $errstr, 30);
		if (!$fp) {
			throw new Exception("StatsRRDB ERROR: Can not connect to '{$this->_mode}://{$this->_host}:{$this->_port}': {$errstr} ({$errno})\n");
		}
		
		// send command and shutdown the socket on our end
		fwrite($fp, $command);
		stream_socket_shutdown($fp, STREAM_SHUT_WR);
		
		// read reply if requested
		if($ignore_reply) {
			fclose($fp);
			return true;
		}
		
		switch($this->_mode) {
		case self::Mode_Udp:
			echo "Reading the data back\n";
			$res = fread($fp, $this->_buf_size);
			echo "Read the data back\n";
			break;
		case self::Mode_Tcp:
			$res = '';
			while (!feof($fp)) {
				$res .= fgets($fp, $this->_buf_size);
			}
			break;
		default:
			throw new Exception("Unknown protocol: " + $this->mode);
		}
				
		// done
		fclose($fp);
		return $res;
	}
	
}; // class StatsRRDB

