
  //OpenEasyTracker config
  
  #define DEBUG 1          //enable debug msg, sent to serial port  

  #define pin_led 13           //digital pin for avr LED, arduino tracker, 2560 - PB7 - 13 (on olimex dev board:=A5)
  long led_interval = 1000;    // interval at which to blink status led (milliseconds) 
   
  //default settings (can be overwritten and stored in EEPRom)
  #define INTERVAL 10000       //how often to collect data (milli sec, 600000 - 10 mins)
  #define INTERVAL_SEND 1      //how many times to collect data before sending (times), sending interval interval*interval_send (4 default)
  #define POWERSAVE 0          //enable powersaving (turn off modem, gps on every loop)
  #define KEY "vZ57vO6tCw"     //key for connection, will be sent with every data transmission
  #define DATA_LIMIT 2500      //current data limit, data collected before sending to remote server can not exceed this
  #define SMS_KEY "pass"       //default password for SMS auth
  
  //settings that can not be overwritten in runtime
  #define HOSTNAME "pool.easytracker.ru"
  #define PROTO "TCP"
  #define PORT "80"
  #define URL "/update.php"   
  
  #define DEFAULT_APN "internet"  //default APN
  #define DEFAULT_USER "guest"  //default APN user
  #define DEFAULT_PASS "guest"  //default APN pass
 
  const char HTTP_HEADER1[ ] = "POST /update.php  HTTP/1.0\r\nHost: pool.easytracker.ru \r\nContent-type: application/x-www-form-urlencoded\r\nContent-length:";  //HTTP header line before length 
  const char HTTP_HEADER2[ ] = "\r\nUser-Agent:EasyTracker1.4\r\nConnection: close\r\n\r\n";        //HTTP header line after length
     
  #define PACKET_SIZE 1400    //TCP data chunk size, modem accept max 1460 bytes per send
  #define PACKET_SIZE_DELIVERY 3000    //in case modem has this number of bytes undelivered, wait till sending new data (3000 bytes default, max sending TCP buffer is 7300)
  
  #define CONNECT_RETRY 5    //how many time to retry connecting to remote server
  
  //SD card config
  #define SDENABLE 1
  #define SDLOGLIMIT 100000   //limit of log file in bytes, new file will be created after this limit reached
  #define SDFILENAME "data" //log files starts from this name, index will be appended (i.e. data0, data1, etc)
  
  //debuging options
  //#define DBG_MEMORY 1   //show free memory (this option ignores main DEBUG setting, requires MemoryFree lib)
  
  
