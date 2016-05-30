# Protocol

## Hostname
Each module generates a "hostname" starting with "ESP-" followed by the 8-digit HEX chip-id. This is used to identify a new unique Module in the MQTT-Tree.

## Default MQTT-Topics
* **/config/<hostname>/online:** 1 - if module in online, 0 - if not (generated through "will message")
* **/config/<hostname>/ipaddr:** IP address of the module
* **/config/<hostname>/roomname:** from here the modules reads its "room name", you whould publish this as retained message

The following topics are only published if the room name is set:
* **/room/<roomname>/<hostname>:** IP address of the module
* **/room/<roomname>/lightX:** turn on, off or toggle light X by publishing "1","on" or "0","off" or "t","toggle" respectively
* **/room/<roomname>/lights:** published state of lights by 0 or 1 seperated by comma
