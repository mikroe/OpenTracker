
  //SD cards functions
  
  void  sd_setup() {
    
    debug_print(F("sd_setup() started"));  
    
    pinMode(C_PWR_SD, OUTPUT);   
    pinMode(SS, OUTPUT);
    
    digitalWrite(C_PWR_SD, HIGH);  //disabled SD

    debug_print(F("sd_setup() completed"));  
    
  }
  
 void  sd_logindex() {
  
   debug_print(F("sd_logindex() started")); 
   
   //set log filename - check if current log is reached the limit
    char tmp[3];      
    itoa(logindex, tmp, 10);
    byte r = 0;
    
    char filename[12] = SDFILENAME;
    int fileindex = 4;

    for(int i=0;i<strlen(tmp);i++)
      {
        filename[fileindex+i] = tmp[i];
      }  
        
   
   if(sd.exists(filename))
    {
       //log file already exists, check it's size        
       if(myFile.open(filename, O_READ))
         {
           debug_print(F("Log filesize:"));
           debug_print(myFile.fileSize());

           debug_print(F("Log limit in bytes:"));
           debug_print(SDLOGLIMIT);
                      
           if(myFile.fileSize() >= SDLOGLIMIT)
             {
              debug_print(F("Log file is above the limit, setting new file name")); 
              
              logindex++;

              debug_print(F("Log file index increased to:")); 
              debug_print(logindex);   
              
              r = 1;
              
             }
             else
             {
              debug_print(F("Log file is within the limit")); 
             }
           
          myFile.close();  
          
          
            //check if new file is ok, up to first 20 files will be checked             
            if((logindex < 40) && (r == 1))    //this can only be executed once, when system started and there are some old files on the card
              {
                debug_print(F("Checking new file for limits")); 
                sd_logindex();   
              }  
               
          
         }  
         else
         {
           debug_print(F("Can not open log file"));  
         }
         
      
    } 
    else
    {
      debug_print(F("Log file does not exists"));   
    }
     
     
   debug_print(F("sd_logindex() completed")); 
   
 } 
  
 void  sd_log() { 
 
   debug_print(F("sd_log() started"));  
   
    //log current data to SD card
    //turn on SD card power
    digitalWrite(C_PWR_SD, LOW);    
    delay(500);
    
    if (!sd.begin(chipSelect, SPI_HALF_SPEED)) {
      debug_print(F("SD initialization failed"));
    }
    else
    {
      debug_print(F("SD card initialized")); 
      
      //set file name - consider max file size
      sd_logindex();    
      
      //building real file name, from index
      char tmp[3];      
      itoa(logindex, tmp, 10);
      
      char filename[12] = SDFILENAME;
      int fileindex = 4;

      for(int i=0;i<strlen(tmp);i++)
        {
          filename[fileindex+i] = tmp[i];
        }  
     
    
      debug_print(F("Saving to file:"));   
      debug_print(filename); 
      
      if(!myFile.open(filename, O_WRITE | O_CREAT | O_AT_END)) 
      {
        debug_print(F("Error opening log file."));  
      }
      else
      {
          debug_print(F("Log file opened."));                 
          myFile.println(data_current);

          debug_print(F("Current data printed to Log file."));                   
          myFile.close();                    
      }
  
  
    }
    
    digitalWrite(C_PWR_SD, HIGH);  //disable SD
    
    debug_print(F("sd_log() completed"));  
 
  }
  
  
    void sd_send_logs()
    {
      char filename[20];
      char filesize[20];
      char c;
      int ret_tmp = 0;
      int delivered = 0;
      int filesent = 0;
                 
      //send all log files from SD card to remove server 
      debug_print(F("sd_send_logs() started"));        
      
      //check how many log files exist
      digitalWrite(C_PWR_SD, LOW);    
      delay(500);
      
      if (!sd.begin(chipSelect, SPI_HALF_SPEED)) {
        debug_print(F("SD initialization failed"));
      }
      else
      {
        debug_print(F("SD card initialized, listing files")); 
        
        while (myFile.openNext(sd.vwd(), O_RDWR)) {          
          
           filesent = 0;
           myFile.getFilename(filename);            
           
           debug_print(F("Found file:"));
           debug_print(filename);
           debug_print(myFile.fileSize()); 

           char *tmp = strstr(filename, "DATA"); 
           if(tmp!=NULL)
           {
              debug_print(F("sd_send_logs(): Found valid file to send."));
                          
              //sending file to remote server, each log file is about 100K

              //send 2 ATs              
              gsm_send_at(); 
             
              //disconnect GSM
              ret_tmp = gsm_disconnect();
              ret_tmp = gsm_connect();
              if(ret_tmp == 1)
               {                 
                 //connected to remote server, sending file
                 
                 //getting total size of the file to be sent
                 ltoa(myFile.fileSize(), filesize, 10);
                 
                 debug_print("sd_send_logs(): Sending file size:");
                 debug_print(filesize);
                 
                 unsigned long http_len = strlen(imei)+strlen(config.key);
                 http_len = http_len+myFile.fileSize();                 
                 
                 http_len = http_len+13;    //imei= &key= &d=                 
                 
                 char tmp_http_len[20];  
                 ltoa(http_len, tmp_http_len, 10);
                 
                 unsigned long tmp_len = strlen(HTTP_HEADER1)+strlen(tmp_http_len)+strlen(HTTP_HEADER2);  
                 
                 //sending header packet to remote host                 
                 gsm_port.print("AT+QISEND=");
                 gsm_port.print(tmp_len); 
                 gsm_port.print("\r");
                 
                 delay(500);
                 gsm_get_reply();
                 
                 //sending header                     
                 gsm_port.print(HTTP_HEADER1); 
                 gsm_port.print(http_len); 
                 gsm_port.print(HTTP_HEADER2);           
                
                 gsm_get_reply();
                 
                 //validate header delivery
                 delivered = gsm_validate_tcp();
                 
                 //do not send other data before current is delivered
                 if(delivered == 1)
                   {             
                       //sending imei and key first
                       gsm_port.print("AT+QISEND=");
                       gsm_port.print(13+strlen(imei)+strlen(config.key)); 
                       gsm_port.print("\r");
                     
                       delay(500);
                       gsm_get_reply();                   
                 
                       gsm_port.print("imei=");
                       gsm_port.print(imei);
                       gsm_port.print("&key=");
                       gsm_port.print(config.key);
                       gsm_port.print("&d=");
                                        
                       delay(500);
                       gsm_get_reply();   
                  
                       //sending file in chunks
                       tmp_len = myFile.fileSize();
                       unsigned long chunk_len;
                       unsigned long chunk_pos = 0;
                       unsigned long chunk_check = 0;
                           
                       if(tmp_len > PACKET_SIZE)
                         {
                           chunk_len = PACKET_SIZE;
                         }
                         else
                         {
                           chunk_len = tmp_len;
                         }
                         
                  
                       int k=0;       
                       for(int i=0;i<tmp_len;i++)
                         {
                           
                             if((i == 0) || (chunk_pos >= PACKET_SIZE))
                              {
                                 
                                debug_print(F("gsm_send_http(): Sending data chunk:"));  
                                debug_print(chunk_pos);
                                                
                                if(chunk_pos >= PACKET_SIZE)
                                  {                                                  
                                     gsm_get_reply();        
                                    
                                     //validate previous transmission  
                                     delivered = gsm_validate_tcp();
                                     
                                     if(delivered == 1)
                                     {
                                          
                                           //next chunk, get chunk length, check if not the last one                            
                                           chunk_check = tmp_len-i;
                      
                                          if(chunk_check > PACKET_SIZE)
                                            {
                                              chunk_len = PACKET_SIZE; 
                                            }
                                            else
                                            {
                                              //last packet
                                              chunk_len = chunk_check;
                                            }
                                                                      
                                          chunk_pos = 0;
                                          
                                     }
                                     else
                                     {
                                       //data can not be delivered, abort the transmission and try again
                                       debug_print(F("sd_send_logs() Can not deliver HTTP data")); 
                                       
                                       break; 
                                     }
                                     
                                  }
                                   
                                    debug_print(F("gsm_send_http(): chunk length:"));  
                                    debug_print(chunk_len);
                                                    
                                    //sending chunk
                                    gsm_port.print("AT+QISEND=");
                                    gsm_port.print(chunk_len); 
                                    gsm_port.print("\r");  
                                    
                                    delay(500);  
                                   
                              }
                
                            //sending data 
                            c = myFile.read();
                            
                            gsm_port.print(c);           
                            chunk_pos++;
                            k++;
                            
                         }
                         
                     //parse server reply, in case #eof received delete file   
                     delay(2000); 
                     filesent = parse_receive_reply();     
                     
                 }  //header delivered
                 else
                 {
                   debug_print(F("sd_send_logs() Can not deliver HTTP header")); 
                 }
                 
                 gsm_get_reply();                 
                 
               }
              
              //disconnecting 
              ret_tmp = gsm_disconnect(); 
             
           }  
           else
           {
             debug_print(F("sd_send_logs(): Skipping file."));
            
           }
           
           
           if(filesent == 1)
             {
               //file has been received by the server, delete the file
               debug_print(F("sd_send_logs(): File has been sent, deleting file."));                
               if(myFile.remove())
                 {
                   debug_print(F("sd_send_logs(): File deleted."));                
                 }
                 else
                 {
                   debug_print(F("sd_send_logs(): File can not be deleted."));                
                 }
             }
             
           myFile.close();  
           
        }
        
        
      }
      
      delay(1000);
      digitalWrite(C_PWR_SD, HIGH);  //disable SD
      
      debug_print(F("sd_send_logs() completed"));  
      
    }
    
