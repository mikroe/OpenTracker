
 void settings_load()
    {
      //load all settings from EEPROM
      debug_print(F("settings_load() started"));
      
      EEPROM_readAnything(0, config);
      
      //setting defaults in case nothing loaded      
      debug_print(F("settings_load(): config.interval:"));
      debug_print(config.interval);    
      
      if((config.interval == -1) || (config.interval == NULL))
        {
          debug_print(F("settings_load(): interval not found, setting default"));
          config.interval = INTERVAL;
          
          debug_print(F("settings_load(): set config.interval:"));
          debug_print(config.interval);    
        }

      
      //interval send
      debug_print(F("settings_load(): config.interval_send:"));
      debug_print(config.interval_send);      

      if((config.interval_send == -1) || (config.interval_send == NULL))
        {
          debug_print(F("settings_load(): interval_send not found, setting default"));
          config.interval_send = INTERVAL_SEND;
          
          debug_print(F("settings_load(): set config.interval_send:"));
          debug_print(config.interval_send);              
        }
      
      //powersave
      debug_print(F("settings_load(): config.powersave:"));
      debug_print(config.powersave);      

      if((config.powersave != 1) && (config.powersave != 0))
        {
          debug_print(F("settings_load(): powersave not found, setting default"));
          config.powersave = POWERSAVE;
          
          debug_print(F("settings_load(): set config.powersave:"));
          debug_print(config.powersave);              
        }
      
      if(config.key[0] == -1)
        {
          debug_print(F("settings_load(): key not found, setting default"));                    
          strlcpy(config.key, KEY, 12);
        }
        
       if(config.sms_key[0] == -1)
        {
           debug_print("settings_load(): SMS key not found, setting default");                 
           strlcpy(config.sms_key, SMS_KEY, 12);
        }         
              
        if(config.apn[0] == -1)
        {
           debug_print("settings_load(): APN not set, setting default");                 
           strlcpy(config.apn, DEFAULT_APN, 20);
        }  

        if(config.user[0] == -1)
        {
           debug_print("settings_load(): APN user not set, setting default");                 
           strlcpy(config.user, DEFAULT_USER, 20);
        }  


        if(config.pwd[0] == -1)
        {
           debug_print("settings_load(): APN password not set, setting default");                 
           strlcpy(config.pwd, DEFAULT_PASS, 20);
        } 
       
      debug_print(F("settings_load() finished"));      
    }
    
 void settings_save()
    {
      debug_print(F("settings_save() started"));
      
      //save all settings to EEPROM
      EEPROM_writeAnything(0, config);
      
      debug_print(F("settings_save() finished"));
    }
    
        
