# ESP8266-Wireless-ArtNet-control
An ESP8266 based ArtNet driver for wireless LED setups based on SmartShow code: https://forum.arduino.cc/index.php?topic=434498.0

Additional features added:
Show red Larson scanner when unable to connect to WiFi
If connected to WiFi and no ArtNet packets received in the last 5 seconds, blink WiFi signal strength:
     red = bad (under -70dB)
     yellow = ok (over -67dB)
     green = good (over -60dB)
     blue = WTF

Based on this guide: https://eyesaas.com/wi-fi-signal-strength/
OTA update capabilities

The basics
Plug your WS2812B strip into pin D1
Set your SSID & password
Set your IP
Power it up

Send ArtNet to that IP

Still to-do:
Would be nice to add a lot of the web admin features from this bin, but in wayyyy simpler form
https://github.com/mtongnz/ESP8266_ArtNetNode_v2

Add web-server for easy on-the-fly config changes for:
SSID
Password
Fixed IP settings
DHCP
ArtNet Universe & channel setups?

Add ArtNet discovery stuff so it will respond when scanned?
