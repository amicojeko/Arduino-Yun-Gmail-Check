/*
  Checking Gmail Inbox with the Process class and the Curl command
 
 This code checks for unread messages under a specific label in the Gmail Inbox
 The label can be specified via REST webservies, thanks to the Bridge Class 
 (included in the process Class) and then saved on a configuration file in the 
 local file system so it will be available after the reboot
 A LED on Pin 13 will inform about the presence of unread messages
 The number of unread messages for the specified label will be displayed on 
 the 7 segments LED display
 
 The circuit:
 * Arduino Yun with LED connected to pin 13 and a MAX7219 based 7 segments LED 
 display on pins 10, 11, 12  
 I used this one: http://dx.com/p/8-segment-led-display-board-module-for-arduino-147814
 
 Code by Stefano Guglielmetti and Massimo Ronca 2013
 
 Based on the ShellCommands Example by Cristian Maglie and Tom Igoe
 and on the LedControl library examples from http://playground.arduino.cc/Main/LedControl
 
 This example code is in the public domain.
 */

#include <Process.h>
#include <FileIO.h>

#include "LedControl.h" // Downloaded From http://playground.arduino.cc/Main/LedControl 


#define DELAY 1000 /* Delay in milliseconds between two loops */
#define MAX_BUFF_LEN 128 // max length in bytes for a buffer
#define LED_PIN 13 /* Led on Pin 13 */


#define CURL "curl"
#define CURL_FLAGS " -k --silent "
#define SETTINGS_URL "https://mail.google.com/mail/feed/atom/"
#define SETTINGS_RE  "grep -oE \"<fullcount>[0-9]*</fullcount>|Error 401\" |grep -oE \"[0-9]*|Error 401\""
#define SETTINGS_CREDENTIALS "gmail.credentials"
#define SETTINGS_LABEL "gmail.label"

#define MAKE_CMD(U,L) "curl -u \"" U.c_str() "\" \"" SETTINGS_URL L.c_str() "\"" CURL_FLAGS " | " SETTINGS_RE

const String SETTINGS_FOLDER="/root/.gmail"; /* This is the settings folder */

String label;
String credentials;
String shell_cmd;
Process proc;

/*
 Now we need a LedControl to work with.
 pin 12 is connected to the DataIn 
 pin 11 is connected to the CLK 
 pin 10 is connected to LOAD 
 We have only a single MAX72XX.
 */

LedControl lc = LedControl(12,11,10,1);


// ** STRING MANIPULATION & HW HANDLING

  void led_on() {
    digitalWrite(LED_PIN, HIGH);     
  }

  void led_off() {
    digitalWrite(LED_PIN, LOW);
  }

// ** SETUP **

void setup() {
  init_board();
  init_console();
  init_led();
  init_fs(); // initialize file system  
  read_settings(); // read credentials and label to watch for
  create_process(); // create command used toretreive messages
}

// ** MAIN FUNCTIONS

  void init_board() {
    Bridge.begin();   /* Initialize the Bridge */
    Serial.begin(9600);   /* Initialize the Serial for debugging */    
  }

  void init_console() {
    Console.begin(); 
  }

  void init_led() {
    pinMode(LED_PIN, OUTPUT); /* Initialize the LED Pin */

    lc.shutdown(0,false); /* The MAX72XX is in power-saving mode on startup, we have to do a wakeup call */
    lc.setIntensity(0,8); /* Set the brightness to a medium values */
    lc.clearDisplay(0);   /* and clear the display */  
  }

  void init_fs() {
    FileSystem.begin();   /* Setup File IO */  
    if(!file_or_dir_exists(SETTINGS_FOLDER))  {
      mkdir(SETTINGS_FOLDER);
    }
  }

  // read initial settings
  void read_settings() {
    credentials = read_credentials();
    label = read_label();
  } 

  // Check if settings have been changed via webservices 
  // Data storage webservices are called this way: 
  // PUT: http://<arduino ip/hostname>/data/put/<key>/<value>  
  // GET: http://<arduino ip/hostname>/data/get/<key>  
  void check_settings() {
    if(credentials_changed() || label_changed()) {
      // is something changed, create a new command with the new params
      create_process();
    }
  }

  // this two functions check for changes in settings, return true on change
  // false otherwise
  boolean credentials_changed() {
    String tmp = get(SETTINGS_CREDENTIALS);

    // Checks if credentials have been changed
    // TODO: If credentials are empty, show an error (NOACCES) on the LED display
    if (tmp.length() > 0 && credentials != tmp){
      credentials = tmp;
      store_credentials(credentials);
      return(true);
    }     
    return(false);
  }

  boolean label_changed() {
    String tmp = get(SETTINGS_LABEL);

    // Checks if a label has been specified via webservices and stores it in 
    // a configuration file. If no label has been specified, it retrieves the 
    // last one from the configuration file.
    // If label file is empty, it checks for INBOX
    if (tmp.length() > 0 && label != tmp){
      label = tmp;
      store_label(label);
      return(true);
    }          
    return(false);
  }

// ** FILE MANIPULATION ROUTINES

  // Check if a file or a directory exists 
  boolean file_or_dir_exists(String file_or_dir) {
    return(FileSystem.exists(file_or_dir.c_str()));
  }

  // Check if a file or a directory exists 
  void mkdir(String dir) {
    FileSystem.mkdir(dir.c_str());
  }

  // Read the content of the file file_name and returns it as a String
  String file_read(String file_name) {
    File file = FileSystem.open(file_name.c_str(), FILE_READ);  
    
    if(!file) {
      //ser_log("File '%s' cannot be opened for reading.", file_name);
      Serial.println("File '" +  file_name + "' cannot be opened for reading");
      return("");
    }
    
    //int bytes_to_read = max(file.size(), MAX_BUFF_LEN); // read up to MAX_BUFF_LEN bytes
    // NOTE: for some reason size is not recognized even if it is documented
    // http://arduino.cc/en/Reference/YunFileIOSize
    // switch to available
    int bytes_to_read = min(file.available(), MAX_BUFF_LEN); // read up to MAX_BUFF_LEN bytes
    char char_buffer[bytes_to_read+1];

    for(int i=0;i<bytes_to_read;++i)
      char_buffer[i] = file.read();

    // remove trailing spaces
    while(isspace(char_buffer[bytes_to_read-1]) && bytes_to_read > 0) 
       bytes_to_read--;
    char_buffer[bytes_to_read] = '\0'; // null terminated string
    
    file.close();

    return(String(char_buffer));
  }

  // Write the String s into the file file_name
  // If file_name already exists, it is overwritten
  void file_write(String file_name, String s) {
    File file = FileSystem.open(file_name.c_str(), FILE_WRITE);
    file.print(s);
    file.close();  
  }

  // Save gmail username and password on persistent storage
  // Credentials are in the form USERNAME:PASSWORD
  void store_credentials(String credentials) {
    file_write(SETTINGS_FOLDER + "/credentials", credentials);
  }

  // Read gmail credentials from persistent storage
  String read_credentials() {
    return(file_read(SETTINGS_FOLDER + "/credentials"));
  }

  // Save gmail label on persistent storage
  void store_label(String label) {
    file_write(SETTINGS_FOLDER + "/label", label);
  }

  // Read gmail label from persistent storage
  String read_label() {
    return(file_read(SETTINGS_FOLDER + "/label"));
  }

// ** PROCESS WRAPPER
  
  int check_for_new_messages() {
    Serial.println("checking for new messages");

    proc.runShellCommand(shell_cmd);

    int bytes_to_read = min(proc.available(), MAX_BUFF_LEN); // read up to MAX_BUFF_LEN bytes
    char char_buffer[bytes_to_read+1];

    for(int i=0;i<bytes_to_read;++i)
      char_buffer[i] = proc.read();

    char_buffer[bytes_to_read] = '\0'; // null terminated string
    proc.flush();
      
    // remove trailing spaces
    while(isspace(char_buffer[bytes_to_read-1]) && bytes_to_read > 0) 
       bytes_to_read--;
    char_buffer[bytes_to_read] = '\0'; // null terminated string

    Serial.println(char_buffer);
    Serial.flush();    

    // check for the string Error 401 for unauthorized calls
    if(strcmp("Error 401", char_buffer)  == 0) {      
      led_clear();
      led_display_string(String("E401"));
      return(-1);
    }
    
    return(atol(char_buffer));
  }

  void create_process() {
    shell_cmd.remove(0); // clear string
    shell_cmd.concat(CURL);
    shell_cmd.concat(CURL_FLAGS);
    shell_cmd.concat(" -u \"");
    shell_cmd.concat(credentials);
    shell_cmd.concat("\" \"");
    shell_cmd.concat(SETTINGS_URL);
    if(label.length() > 0)
      shell_cmd.concat(label);
    shell_cmd.concat("\" | ");
    shell_cmd.concat(SETTINGS_RE);
  }


// ** 7 SEGMENTS LED WRAPPER
  
  void led_clear() {
    lc.clearDisplay(0);    
  }

  // This is the printNumber function for the LED Display, borrowed from 
  // http://playground.arduino.cc/Main/LedControl#NumberSeg7Control
  void led_display_number(int n) {
    int digit_pos = 0;

    if(n == 0) {
      lc.setDigit(0,0,0,false);
    } else {
      while (n > 0) {
        int digit = n % 10;
        // do something with digit
        lc.setDigit(0,digit_pos,(byte)digit,false);

        n /= 10;
        digit_pos++;
      }        
    }
  }
  
  void led_display_string(String s) {
    if(s.length() == 0)
      return;

    int digit = s.length()-1;
    for(int i=0;i<s.length();++i,digit--) {
      lc.setChar(0, digit, s.charAt(i), false);
    }

  }

// ** INTERNAL STORAGE WRAPPER

  String get(String key) {
    char char_buffer[MAX_BUFF_LEN];
    Bridge.get(key.c_str(), char_buffer, MAX_BUFF_LEN);
    return(String(char_buffer)); 
  }

// ** MAIN LOOP 
void loop() {

  check_settings();

  int messages = check_for_new_messages();
  
  if(messages >= 0) {
    led_clear();
    led_display_number(messages);
  }

  if (messages > 0)
    led_on(); /* If I got messages, then I turn the red LED on */
  else
    led_off(); /* No messages, so I turn the red LED off */

  delay(DELAY);  // wait 5 seconds before you do it again

}








