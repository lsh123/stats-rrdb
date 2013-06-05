<?php
$server = '127.0.0.1';
$port   = 9876;
$input  = count($argv) > 1 ? $argv[1] : '';


$sock = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
if(!$sock) {
  $errorcode = socket_last_error();
  $errormsg = socket_strerror($errorcode);
  die("Could not send data: [$errorcode] $errormsg \n");
} 
echo "Socket created\n";
     
// Send the message to the server
$res = socket_sendto($sock, $input , strlen($input) , 0 , $server , $port);
if(!$res) {
  $errorcode = socket_last_error();
  $errormsg = socket_strerror($errorcode);
         
  die("Could not send data: $errorcode - $errormsg\n");
}
echo "Message sent: '{$input}'\n";
         
// Now receive reply from server and print it
$res = socket_recv ( $sock , $reply , 2045 , MSG_WAITALL );
if($res == FALSE) {
  $errorcode = socket_last_error();
  $errormsg = socket_strerror($errorcode);
         
  die("Could not receive data: $errorcode - $errormsg\n");
}
echo "Reply: '{$reply}\n";


