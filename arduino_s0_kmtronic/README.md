This project allows you via HTTP to see how many s0 pulses have been detected on input1 and input2 of the KMtronic Arduino DINo https://sigma-shop.com/product/73/web-internet-ethernet-controlled-relay-board-arduino-compatible-rs485-usb-boxed-for-din-mount-rail.html

Use a 10k Ohm pullup-resistor to +12V and connect the s0 output from the electricity meter etc. Since you only get the count of s0 pulses since last reboot of the Arduino, a script that fetches the values must make the calculations.
