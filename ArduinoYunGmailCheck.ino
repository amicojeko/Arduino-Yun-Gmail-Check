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

#include <stdarg.h>
#include <Process.h>
#include <FileIO.h>

#include "LedControl.h" // Downloaded From http://playground.arduino.cc/Main/LedControl 


#define DELAY 5000 /* This is the settings folder */
#define MAX_BUFF_LEN 512 // max length in bytes for a buffer
#define LED_PIN 13 /* Led on Pin 13 */

#define SETTINGS_CREDENTIALS "gmail.credentials"
#define SETTINGS_LABEL "gmail.label"

const String SETTINGS_FOLDER="/root/.gmail"; /* This is the settings folder */

// generic buffer that will temporary hold the values from thoose functions that 
// read into char arrays. used to avoid to allocate and release heap memory
char buffer[MAX_BUFF_LEN]; 

String label;
String credentials;

/*
 Now we need a LedControl to work with.
 pin 12 is connected to the DataIn 
 pin 11 is connected to the CLK 
 pin 10 is connected to LOAD 
 We have only a single MAX72XX.
 */

LedControl lc = LedControl(12,11,10,1);


// ** STRING MANIPULATION & HW HANDLING

  // log something on Serial, using a printf format string
  void ser_log(char *fmt, ... ){
    va_list args;
    va_start (args, fmt );
      vsnprintf(buffer, MAX_BUFF_LEN, fmt, args);
    va_end (args);
    Serial.print(buffer);
  }

  // format a string using printf format string
  String fmt(char *fmt, ... ) {
    va_list args;
    va_start (args, fmt );
      vsnprintf(buffer, MAX_BUFF_LEN, fmt, args);
    va_end (args);
    return(String(buffer));
  } 

  void led_on() {
    digitalWrite(LED_PIN, HIGH);     
  }

  void led_off() {
    digitalWrite(LED_PIN, LOW);
  }

// ** SETUP **

void setup() {
  init_board();
  init_led();
  init_fs(); // initialize file system  
  read_settings();
}

// ** MAIN FUNCTIONS

  void init_board() {
    Bridge.begin();   /* Initialize the Bridge */
    Serial.begin(9600);   /* Initialize the Serial for debugging */
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
    ser_log("reading settings... ");
    
    credentials = read_credentials();
    label = read_label();

    ser_log("done!\n");  
  } 

  // Checks if a settings have been chenged via webservices 
  // storage webservices are called this way: 
  // http://<arduino ip/hostname>/data/put/<key>/<value>  
  void check_settings() {
    check_credentials();
    check_label();
  }

  void check_credentials() {
    String tmp = get(SETTINGS_CREDENTIALS);

    // Checks if credentials have been changed
    // TODO: If credentials are empty, show an error (NOACCES) on the LED display
    if (tmp.length() > 0 && credentials != tmp){
      credentials = tmp;
      store_credentials(credentials);
    }     
  }

  void check_label() {
    String tmp = get(SETTINGS_LABEL);

    // Checks if a label has been specified via webservices and stores it in 
    // a configuration file. If no label has been specified, it retrieves the 
    // last one from the configuration file.
    // If label file is empty, it checks for INBOX
    if (tmp.length() > 0 && label != tmp){
      label = tmp;
      store_label(label);
    }     
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
    
    //int bytes_to_read = max(file.size(), MAX_BUFF_LEN); // read up to MAX_BUFF_LEN bytes
    // NOTE: for some reason size is not recognized even if it is documented
    // http://arduino.cc/en/Reference/YunFileIOSize
    // switch to available
    int bytes_to_read = max(file.available(), MAX_BUFF_LEN); // read up to MAX_BUFF_LEN bytes

    for(int i=0;i<bytes_to_read;++i)
      buffer[i] = file.read();

    buffer[bytes_to_read] = '\0'; // null terminated string
    file.close();

    return(String(buffer));
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
    // Checks for unread messages in a specified label and returns the number of messages 
    String shell_cmd = fmt("curl -u \"%s\" \"https://mail.google.com/mail/feed/atom/%s\" \
      -k --silent |grep -o \"<fullcount>[0-9]*</fullcount>\" |grep -o \"[0-9]*\"", \
      credentials, label);
 
    // This command checks for a specified sender and returns the number of messages, 
    // i changed it to check for a label because I thought the label was more 
    // flexible as I could configure it from Gmail, 
    // I left it here for your pleasure
    // curl -u USERNAME:PASSWORD "https://mail.google.com/mail/feed/atom" -k --silent |grep -o "<email>EMAIL_ADDR</email>" |wc -l 

    return(run_shellcmd_with_retval(shell_cmd));
  }

  int run_shellcmd_with_retval(String cmd) {

    Process p;
    p.runShellCommand(cmd);
    while(p.running());  /* do nothing until the process finishes, so you get the whole output */

    int result = p.parseInt();  /* look for an integer */
    p.flush();

    return(result);
  }

  // This is the printNumber function for the LED Display, borrowed from 
  // http://playground.arduino.cc/Main/LedControl#NumberSeg7Control
  void print_number(int n) {
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
  
// ** INTERNAL STORAGE WRAPPER

  String get(String key) {
    Bridge.get(key.c_str(), buffer, MAX_BUFF_LEN);
    return(String(buffer)); 
  }

// ** MAIN LOOP 
void loop() {

  check_settings();

  int messages = check_for_new_messages();
  
  print_number(messages);

  if (messages > 0)
    led_on(); /* If I got messages, then I turn the red LED on */
  else
    led_off(); /* No messages, so I turn the red LED off */

  delay(DELAY);  // wait 5 seconds before you do it again
}
