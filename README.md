Arduino-Yun-Gmail-Check
=======================

Checking Gmail Inbox with the Process class and the Curl command
 
 This code checks for unread messages under a specific label in the Gmail Inbox
 The label can be specified via REST webservies, thanks to the Bridge Class (included in the process Class)
 and then saved on a configuration file in the local file system so it will be available after the reboot
 A LED on Pin 13 will inform about the presence of unread messages
 The number of unread messages for the specified label will be displayed on the 7 segments LED display
 
 The circuit:
 Arduino Yun with LED connected to pin 13 and a MAX7219 based 7 segments LED display
 on pins 10, 11, 12  
 I used this one: http://dx.com/p/8-segment-led-display-board-module-for-arduino-147814
 
 Code by Stefano Guglielmetti and Massimo Ronca 2013
 
 Based on the ShellCommands Example by Cristian Maglie and Tom Igoe
 and on the LedControl library examples from http://playground.arduino.cc/Main/LedControl
 
 This example code is in the public domain.
 
 
![Photo](http://i.imgur.com/ZJb97bol.jpg "Arduino Yun Gmail Alerter") 
 
 
##Instructions

###Software Setup

In order for your Arduino to be able to access your Gmail feeds, you need to supply valid Gmail credentials.
The credentials are in the form used by Basic HTTP Auth, USERNAME:PASSWORD, for example `john:doe`.

All you need to do is just call the Yun web service for temporary storage at this address 
http://arduino.local/data/put/gmail.credentials/USERNAME:PASSWORD
replacing USERNAME and PASSWORD with you real credentials.
A second way is to directly modify/create the configuration files used to persist the settings.
There are two files, placed inside the `.gmail` folder in the root home, one called `credentials`, one called `label`.
Not surprisingly, the first one contains the credentials to access Gmail and the second the label you want to monitor.
Both these settings are hot reloaded on change.
Changes through webservices are also persisted on disk.

Settings are persistent, once configured, they will survive a reboot or a reset.

You can now turn Arduino Yun on **Arduino Yun must be connected to the internet**

###Hardware

Put a led on Arduino's digital pin 13

If you want to add the 7 Segments LED Display, use a MAX7129 display (I used http://dx.com/p/8-segment-led-display-board-module-for-arduino-147814)

and connect

* pin 12 to the DataIn 
* pin 11 to the CLK 
* pin 10 to LOAD 
 
plus, obviously, +5V and GND :)

![the hardware](http://i.imgur.com/eiaMH2Gl.jpg "Arduino Yun Gmail Alerter - the hardware")

###Usage

Configure a label on Gmail, apply some filters, then point your browser to http://<arduino host>/data/put/gmail.label/LABELNAME
and replace LABELNAME with your label. Arduino will turn the led ON if there are unread messages under that label, and the LED display will show the number of unread messages.
