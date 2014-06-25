/*    
    
    Copyright (c) 2014 "OOO NPF InPromTech", Russia [www.inpromtech.ru]
    Copyright (c) 2014 "Tigal KG", Austria [www.tigal.com]

    This software is the result of Russia-Austria partnership and open source community efforts.    
    This software was initially developed by Andrey Sheykhot [andrey@sheykhot.com]

    This file is part of OpenEasyTracker Software version 1.  

    OpenEasyTracker Software is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as published by
    the Free Software Foundation.

    OpenEasyTracker Software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenEasyTracker Software.  If not, see <http://www.gnu.org/licenses/>.
*/

  //tracker config
  #include "tracker.h"   
  
  //External libraries
  #include <TinyGPS.h>
  #include <EEPROM.h>
  #include "EEPROMAnything.h"   
  #include <SdFat.h> 
  
  //memory debug
  #ifdef DBG_MEMORY    
    #include <MemoryFree.h>
  #endif

  //AVR watchdog 
  #include <avr/wdt.h> 

  
  #ifdef DEBUG
    #define debug_print(x)  debug_port.println(x)
  #else
    #define debug_print(x)
  #endif

  // Variables will change:
  int ledState = LOW;             // ledState used to set the LED
  long previousMillis = 0;        // will store last time LED was updated
  long watchdogMillis = 0;        // will store last time modem watchdog was reset
  
  long time_start, time_stop, time_diff;             //count execution time to trigger interval
  
  int interval_count = 0;         //current interval count (increased on each data collection and reset after sending)
   
  
  char data_current[DATA_LIMIT];   //data collected in one go, max 2500 chars 
  int data_index = 0;        //current data index (where last data record stopped)
  char imei[20];             //IMEI number
  char time[20];             //time attached to every data line
  char modem_reply[200];     //data received from modem, max 200 chars   
  int logindex = 0;
  byte save_config = 0;      //flag to save config to EEPROM
  byte power_reboot = 0;           //flag to reboot everything (used after new settings have been saved)
  
  unsigned long last_time_gps, last_date_gps;      
  TinyGPS gps;
 
  // SD chip select pin
  const uint8_t chipSelect = SS;
  SdFat sd;
  SdFile myFile;
   
   //settings structure  
  struct settings {
  
    char apn[20];
    char user[20];
    char pwd[20];
    long interval;          //how often to collect data (milli sec, 600000 - 10 mins)
    int interval_send;      //how many times to collect data before sending (times), sending interval interval*interval_send
    byte powersave;
    char key[12];           //key for connection, will be sent with every data transmission
    char sim_pin[5];        //PIN for SIM card
    char sms_key[12];       //password for SMS commands
    
  };

  settings config;
  
  //define serial ports, total 4 hardware ports  
  HardwareSerial& gps_port = Serial2;
  HardwareSerial& debug_port = Serial;
  HardwareSerial& aux_port = Serial3; 
  HardwareSerial& gsm_port = Serial1;
   

  void setup() {
  
    wdt_reset(); 
    wdt_disable();
    
    //setting serial ports
    gsm_port.begin(9600);  
    debug_port.begin(9600);  
    gps_port.begin(9600);  
    aux_port.begin(9600);  
    
    debug_print(F("setup() started"));
    
    settings_load();
    
    //setting pins
    //setup led pin
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(C_REBOOT, OUTPUT);
    
    //blink software start
    blink_start();   
  
    //GPS setup 
    gps_setup();
    gps_on_off();
    
    //GSM setup
    gsm_setup();
    
    //turn on GSM
    gsm_restart();
    
    //send AT
    gsm_send_at();
    delay(1000);
    gsm_send_at();
    delay(1000);
    
    //supply PIN code is needed 
    gsm_set_pin();
    
    //get GSM IMEI
    gsm_get_imei();
    
    //misc GSM startup commands (disable echo)
    gsm_startup_cmd();
  
  #ifdef SDENABLE
    //SD card setup
    sd_setup();
  #endif    
  
    //set GSM APN
    gsm_set_apn();    
    
    //set to connect once started
    interval_count = config.interval_send;
    
    debug_print(F("setup() completed"));
     
  }

    
  
  void loop() {
    
    debug_print(F("loop started"));
    
    //start counting time
    time_start = millis();  
    
    //gsm_debug();
     
     //debug       
     if(data_index >= DATA_LIMIT)
       {
        data_index = 0;
       }        
    
   //blink led  
   status_led();
     
 #ifdef DBG_MEMORY    
   debug_port.print(F("MEMORY:")); 
   debug_port.println(getFreeMemory());
   delay(3000);
 #endif   
     
   
   
   sms_check();   
   
   //collecting GPS data
   collect_all_data();
 
   debug_print(F("Current:")); 
   debug_print(data_current); 
  
    
   int i = gsm_send_data();         
   if(i != 1)
    {
      //current data not sent, save to sd card  
      debug_print(F("Can not send data, saving to SD card"));   

   #ifdef SDENABLE   
      sd_log();   //in case this fails - data is lost
   #endif 
   
    }
    else
    {
      debug_print(F("Data sent successfully."));
    }
   
    //reset current data and counter
    data_index = 0;       
  
  #ifdef SDENABLE
    //send available log files    
    sd_send_logs();
  #endif    
  
 
  
    if(save_config == 1)
       {
         //config should be saved
         settings_save();
         save_config = 0; 
       }
         
     if(power_reboot == 1)
       {
         //reboot unit
         reboot(); 
         power_reboot = 0;        
       }  
     
    
     time_stop = millis();  
     if(time_stop > time_start)
       {
         //check how many  
         time_diff = time_stop-time_start;
         time_diff = config.interval-time_diff;         
         
         debug_print(F("Sleeping for:"));
         debug_print(time_diff);
         debug_print(F("ms"));
         
         if(time_diff > 0)
           {
             delay(time_diff);
           }
           else 
           {
             debug_print(F("Error: negative sleep time."));
             delay(500); 
           }
       }
       else
       {
         //probably counter reset, 50 days passed
        debug_print(F("Time counter reset")); 
        delay(1000); 
       }
    
    
  }
