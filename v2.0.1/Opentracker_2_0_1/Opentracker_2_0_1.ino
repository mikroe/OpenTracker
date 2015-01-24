
  //tracker config
  #include "tracker.h" 
  
  //External libraries
  #include <TinyGPS.h>  
  #include <avr/dtostrf.h>
  
  #include <DueFlashStorage.h>

  
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
  char time_char[20];             //time attached to every data line
  char modem_reply[200];     //data received from modem, max 200 chars   
  long logindex = STORAGE_DATA_START;
  byte save_config = 0;      //flag to save config to flash
  byte power_reboot = 0;           //flag to reboot everything (used after new settings have been saved)


  unsigned long last_time_gps, last_date_gps;      
  
  TinyGPS gps;  
  DueFlashStorage dueFlashStorage;
  

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
    char imei[20];          //IMEI number

    
  };

  settings config;
  
  //define serial ports 
  #define gps_port Serial1
  #define debug_port SerialUSB
  #define gsm_port Serial2
  

void setup() {
 
	////////////// watchdog ///////////////////
	
	// remember that watchdog is enabled by default, calling WDT_Disable() here will disable it. If you comment out WDT_Disable(), 
	// your code should call WDT_Restart() before the default timeout is reached (16s).
	WDT_Disable();

	// use the following code to change the watchdog default timeout (16s). Set wdp_ms as follows: {your intended timeout} * 256
	//uint32_t wdp_ms = 4096;
	//WDT_Enable(WDT, 0x2000 | wdp_ms | (wdp_ms << 16)); // can be called only once after the board starts
	
	// use WDT_Restart(WDT) in your code to reset the watchdog timer
	
	////////////// watchdog ///////////////////
   
    //setting serial ports
    gsm_port.begin(115200);  
    debug_port.begin(9600);  
    gps_port.begin(9600);  
    
    //setup led pin
    pinMode(PIN_POWER_LED, OUTPUT);
    digitalWrite(PIN_POWER_LED, LOW); 
    
    pinMode(PIN_C_REBOOT, OUTPUT);
    digitalWrite(PIN_C_REBOOT, LOW);  //this is required
    
    debug_print(F("setup() started"));
     
    //blink software start
    blink_start();   
  
    //debug remove
    delay(5000);   

    settings_load();
    
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

    //set GSM APN
    gsm_set_apn();    
    
    //get current log index
    #ifdef STORAGE
      storage_get_index();
    #endif     
    
    //setup ignition detection
    
    pinMode(PIN_S_DETECT, INPUT);
    
    //set to connect once started
    interval_count = config.interval_send;    
    
     
    debug_print(F("setup() completed"));

}

void loop() { 
  
  int IGNT_STAT;
  

    //start counting time
    time_start = millis();  
    
       //debug       
     if(data_index >= DATA_LIMIT)
       {
        data_index = 0;
       }        
    
    status_led();
    sms_check();   


  // Check if ignition is turned on
  IGNT_STAT = digitalRead(PIN_S_DETECT);
  debug_print(F("Ignition status:"));
  debug_print(IGNT_STAT);
 
 if(IGNT_STAT == 0)
 {
 debug_print(F("Ignition is ON!"));
 // Insert here only code that should be processed when Ignition is ON

   //collecting GPS data
   collect_all_data();
   debug_print(F("Current:")); 
   debug_print(data_current); 
   
   int i = gsm_send_data();         
   if(i != 1)
    {
      //current data not sent, save to sd card  
      debug_print(F("Can not send data, saving to flash memory"));
/*
       #ifdef STORAGE
          storage_save_current();   //in case this fails - data is lost
       #endif   
    
*/
    }
    else
    {
      debug_print(F("Data sent successfully."));
    }
   
   /*
   #ifdef STORAGE
    //send available log files    
    storage_send_logs();
   
   //debug
   storage_dump();
  #endif   
   */
   
    //reset current data and counter
    data_index = 0;  

}
else {
  debug_print(F("Ignition is OFF!"));
 // Insert here only code that should be processed when Ignition is OFF 


}

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
     
      sms_check(); 
    
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
             debug_print(F("Error: negative sleep time. Interval shorter than processing time"));
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
