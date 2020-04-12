This project allows one to request meter values for Eastron SDM elecitricity meters via rs485 Modbus RTU and control the four build in relays for the KMtronic Arduino DINo https://sigma-shop.com/product/73/web-internet-ethernet-controlled-relay-board-arduino-compatible-rs485-usb-boxed-for-din-mount-rail.html
Furthermore some buttons can be attached, to turn on and of relay 2 locally (a special case I needed).

Example URL's

 * http://arduino/el/17/human1 get various values from single phase meter on modbus address 17
 * http://arduino/el/1/human3 get various values from three phase meter on modbus address 1
 * http://arduino/el/1/kwh get only kWh reading for meter with modbus address 1
 * http://arduino/relay/1/status get status for relay 1
 * http://arduino/relay/2/0 set relay 2 to off
 * http://arduino/relay/3/1 set relay 3 to on
 * http://arduino/input/3 status for input 3 (button on or off)
