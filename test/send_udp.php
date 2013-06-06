<?php
$server = '127.0.0.1';
$port   = 9876;
$input  = count($argv) > 1 ? $argv[1] : '';
$buf_size = 1024;

$fp = stream_socket_client("udp://{$server}:{$port}", $errno, $errstr);
if (!$fp) {
    die("ERROR: {$errstr} ({$errno})\n");
}

fwrite($fp, $input);
echo "Wrote:\n{$input}\n";
echo "Reply:\n";
echo fread($fp, $buf_size);
fclose($fp);
echo "\nDone\n";


