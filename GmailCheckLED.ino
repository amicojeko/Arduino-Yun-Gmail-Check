/*
  Checking Gmail Inbox with the Process class and the Curl command
 
 This code checks for unread messages under a specific label in the Gmail Inbox
 The label can be specified via REST webservies, thanks to the Bridge Class (included in the process Class)
 and then saved on a configuration file in the local file system so it will be available after the reboot
 A LED on Pin 13 will inform about the presence of unread messages
 The number of unread messages for the specified label will be displayed on the 7 segments LED display
 
 The circuit:
 * Arduino Yun with LED connected to pin 13 and a MAX7219 based 7 segments LED display
 on pins 10, 11, 12  
 I used this one: http://dx.com/p/8-segment-led-display-board-module-for-arduino-147814
 
 Code by Stefano Guglielmetti and Massimo Ronca 2013
 
 Based on the ShellCommands Example by Cristian Maglie and Tom Igoe
 and on the LedControl library examples from http://playground.arduino.cc/Main/LedControl
 
 This example code is in the public domain.
 */

#include <Process.h>
#include <FileIO.h>
#include "LedControl.h" /* Downloaded From http://playground.arduino.cc/Main/LedControl */

const int ledPin =  13; /* Led on Pin 13 */
const char* settings_file = "/root/gmail_settings\0"; /* This is the settings file */

char labelbuffer[256]; /* We will need a buffer for getting the label parameter via Bridge's REST APIs */
String label;

/* GMAIL SETTINGS */

const String username = "USERNAME";
const String password = "PASSWORD";

/*
 Now we need a LedControl to work with.
 pin 12 is connected to the DataIn 
 pin 11 is connected to the CLK 
 pin 10 is connected to LOAD 
 We have only a single MAX72XX.
 */

LedControl lc=LedControl(12,11,10,1);


unsigned long delaytime=250; /* we always wait a bit between updates of the display */

void setup() {
  pinMode(ledPin, OUTPUT); /* Initialize the LED Pin */

  lc.shutdown(0,false); /* The MAX72XX is in power-saving mode on startup, we have to do a wakeup call */
  lc.setIntensity(0,8); /* Set the brightness to a medium values */
  lc.clearDisplay(0);   /* and clear the display */

  Bridge.begin(); 	/* Initialize the Bridge */
  Serial.begin(9600); 	/* Initialize the Serial for debugging */
  FileSystem.begin();   /* Setup File IO */
}

void loop() {
/* Checks if a label has been passed via webservices 
  the call is like http://arduino.local/data/put/label/LABEL
*/
  Bridge.get("label", labelbuffer, 256); 


  /*
    Checks if a label has been specified via webservices and stores it on a configuration file
   If a label has not been specified, it retrieves the last on from the configuration file
   */

  if (String(labelbuffer).length() > 0 && label != String(labelbuffer)){
    label = String(labelbuffer);
    File settings = FileSystem.open(settings_file, FILE_WRITE);
    settings.print(label);
    settings.close();
  } 
  else {
    label = "";
    File settings = FileSystem.open(settings_file, FILE_READ);
    while (settings.available() > 0){
      char c = settings.read();
      label += c;
    }
    settings.close();
  }
  
  Serial.println("label:" + label);

  Process p;

  /* 
   This command checks for a specified sender and returns the number of messages, 
   i changed it to check for a label because I thought the label was more flexible as I could configure it from Gmail, 
   but I leave it here for your pleasure
   p.runShellCommand("curl -u USERNAME:PASSWORD \"https://mail.google.com/mail/feed/atom\" -k --silent |grep -o \"<email>" + String(email_addr) + "</email>\"|wc -l"); 
   */


  /* This command checks for a specified label and returns the number of messages */
  p.runShellCommand("curl -u " + username + ":" + password + " \"https://mail.google.com/mail/feed/atom/" + label + "\" -k --silent |grep -o \"<fullcount>[0-9]*</fullcount>\" |grep -o \"[0-9]*\"");

  while(p.running());  /* do nothing until the process finishes, so you get the whole output */


  /* Read command output. runShellCommand() should have passed an integer number" */
  int result = p.parseInt();  /* look for an integer */
  Serial.print("Result:"); /* Some serial debugging */
  Serial.println(result);
  printNumber(result); // print the number on the LED Display
  p.flush();

  if (result > 0)
    digitalWrite(ledPin, HIGH); /* If I got messages, then I turn on the LED */
  else
    digitalWrite(ledPin, LOW); /* No messages, so I turn the LED off */

  delay(5000);  // wait 5 seconds before you do it again
}


/* 
 This is the printNumber function for the LED Display, borrowed from 
 http://playground.arduino.cc/Main/LedControl#NumberSeg7Control
 */

void printNumber(int v) {
  int ones;
  int tens;
  int hundreds;
  boolean negative = false;	

  if(v < -999 || v > 999) 
    return;
  if(v<0) {
    negative=true;
    v=v*-1;
  }

  ones=v%10;
  v=v/10;
  tens=v%10;
  v=v/10;
  hundreds=v;			
  if(negative) {
    lc.setChar(0,3,'-',false); /* print character '-' in the leftmost column */
  }
  else {
    lc.setChar(0,3,' ',false); /* print a blank in the sign column */
  }
  //Now print the number digit by digit
  lc.setDigit(0,2,(byte)hundreds,false);
  lc.setDigit(0,1,(byte)tens,false);
  lc.setDigit(0,0,(byte)ones,false);
}







