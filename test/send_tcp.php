<?php
$server = '127.0.0.1';
$port   = 9876;
$input  = count($argv) > 1 ? $argv[1] : '';
$buf_size = 1024;

$fp = stream_socket_client("tcp://{$server}:{$port}", $errno, $errstr, 30);
if (!$fp) {
    die("ERROR: {$errstr} ({$errno})\n");
}

fwrite($fp, $input);
echo "Wrote:\n{$input}\n";
stream_socket_shutdown($fp, STREAM_SHUT_WR);

echo "Reply:\n";
while (!feof($fp)) {
   echo fgets($fp, $buf_size);
}
fclose($fp);
echo "\nDone\n";

