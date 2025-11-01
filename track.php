<?php
// track.php
$API_KEY = "MYSECRET123";
if (!isset($_GET['key']) || $_GET['key'] !== $API_KEY) {
  http_response_code(403);
  exit("Invalid key");
}
$lat = $_GET['lat'];
$lng = $_GET['lng'];
$spd = $_GET['spd'];
$time = date('Y-m-d H:i:s');

// Save to file (or DB)
$log = "$time,$lat,$lng,$spd\n";
file_put_contents("locations.csv", $log, FILE_APPEND | LOCK_EX);

echo "OK";
?>
