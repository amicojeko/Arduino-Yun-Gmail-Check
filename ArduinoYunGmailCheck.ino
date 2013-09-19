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


#define CREDENTIALS "credentials"
#define LABEL "label"

#define CURL "curl"
#define CURL_FLAGS " -k --silent "
#define SETTINGS_URL "https://mail.google.com/mail/feed/atom/"
#define SETTINGS_RE  "grep -oE \"<fullcount>[0-9]*</fullcount>|Error 401\" |grep -oE \"[0-9]*|Error 401\""
#define SETTINGS_KEY_PREFIX "gmail."
#define SETTINGS_KEY_CREDENTIALS SETTINGS_KEY_PREFIX CREDENTIALS
#define SETTINGS_KEY_LABEL SETTINGS_KEY_PREFIX LABEL

const String SETTINGS_FOLDER = "/root/.gmail"; /* This is the settings folder */

String label; 
String credentials;

// monitor changes in settings filles
int label_mtime; 
int credentials_mtime;

String shell_cmd;

/*
 Now we need a LedControl to work with.
 pin 12 is connected to the DataIn 
 pin 11 is connected to the CLK 
 pin 10 is connected to LOAD 
 We have only a single MAX72XX.
 */

LedControl lc = LedControl(12,11,10,1);

/*
 * --------------
 *     SETUP 
 * --------------
 */

void setup() {
  init_board();
  init_console();
  init_led();
  init_fs(); // initialize file system  
  // init_settings()); // read credentials and label to watch for
  init_process(); // create command used toretreive messages
}

/*
 * --------------
 * INIT FUNCTIONS 
 * --------------
 */

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
  void init_settings() {
    credentials = settings_read(CREDENTIALS);
    credentials_mtime = settings_mtime(CREDENTIALS);

    label = settings_read(LABEL);
    label_mtime = settings_mtime(LABEL);
  } 

/*
 * ---------------------------------
 * STRING MANIPULATION & HW HANDLING
 * ---------------------------------
 */

  void led_on() {
    digitalWrite(LED_PIN, HIGH);     
  }

  void led_off() {
    digitalWrite(LED_PIN, LOW);
  }

/*
 * ------------------
 * SETTINGS FUNCTIONS
 * ------------------
 */

  // read and write settings on disk and memory
  // gmail username and password are in the form USERNAME:PASSWORD
  // label is just te label string
  void settings_store(String key, String value) {    
    put(key, value); // store is permanent, so save on both disk and memory
    file_write(SETTINGS_FOLDER + "/" + key, value);
  }

  String settings_read(String key) {
    return(file_read(SETTINGS_FOLDER + "/" + key));
  }

  // get modification time of a settings file
  int settings_mtime(String key) {
    return(file_mtime(SETTINGS_FOLDER + "/" + key));
  }

  // Check if settings have been changed via webservices 
  // Data storage webservices are called this way: 
  // PUT: http://<arduino ip/hostname>/data/put/<key>/<value>  
  // GET: http://<arduino ip/hostname>/data/get/<key>  
  void settings_check() {
    Serial.println(label);
    Serial.println(credentials);
    Serial.println("----------");
    if(settings_changed()) {
      // is something changed, create a new command with the new params
      init_process();
    }
  }

  // this two functions check for changes in settings, return true on change
  // false otherwise
  boolean settings_changed() {
    String value;
    int mtime;
    boolean changed = false;

    // check LABEL
    value = get(SETTINGS_KEY_LABEL);
    if (value.length() > 0 && value != label) {
      label = value;
      settings_store(SETTINGS_KEY_LABEL, value);
      changed = true;
    } else if(label_mtime != (mtime = settings_mtime(SETTINGS_KEY_LABEL))) {
      Serial.println("label file changed");
      label = settings_read(SETTINGS_KEY_LABEL);
      label_mtime = mtime;
      settings_store(SETTINGS_KEY_LABEL, label);
      changed = true;
    }    

    // check CREDENTIALS
    value = get(SETTINGS_KEY_CREDENTIALS);
    if (value.length() > 0 && value != credentials) {
      credentials = value;
      settings_store(SETTINGS_KEY_CREDENTIALS, value);
      changed = true;
    } else if(credentials_mtime != (mtime = settings_mtime(SETTINGS_KEY_CREDENTIALS))) {
      Serial.println("credentials file changed");
      credentials = settings_read(SETTINGS_KEY_CREDENTIALS);
      credentials_mtime = mtime;
      settings_store(SETTINGS_KEY_CREDENTIALS, credentials);
      changed = true;
    }    


    return(changed);
  }

/*
 * --------------------------
 * FILE MANIPULATION ROUTINES
 * --------------------------
 */

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
    Serial.print("file_read: ");
    Serial.println(file_name);
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

  int file_mtime(String file_name) {
    String cmd = "date +%s --reference=";
    cmd.concat(file_name);

    Process p;    
    p.runShellCommand(cmd);
    
    return(process_read(p).toInt());
  }


// ** PROCESS WRAPPER


  int check_for_new_messages() {
    Serial.println("checking for new messages");

    Process p;
    p.runShellCommand(shell_cmd);

    String s = process_read(p);

    Serial.println(s);
    Serial.flush();    

    // check for the string Error 401 for unauthorized calls
    if(s == "Error 401") {      
      led_clear();
      led_display_string(String("E401"));
      return(-1);
    }
    
    return(s.toInt());
  }

  String process_read(Process p) {

    int bytes_to_read = min(p.available(), MAX_BUFF_LEN); // read up to MAX_BUFF_LEN bytes
    char char_buffer[bytes_to_read+1];

    for(int i=0;i<bytes_to_read;++i)
      char_buffer[i] = p.read();

    char_buffer[bytes_to_read] = '\0'; // null terminated string
    p.flush();
      
    // remove trailing spaces
    while(isspace(char_buffer[bytes_to_read-1]) && bytes_to_read > 0) 
       bytes_to_read--;
    char_buffer[bytes_to_read] = '\0'; // null terminated string

    return(String(char_buffer));
  }

  void init_process() {
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

  void put(String key, String value) {
    Bridge.put(key, value);
  }

// ** MAIN LOOP 
void loop() {

  settings_check();

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








