//*************************************************************************
 OpenMultiCryptoATM
 (ver. 1.5.4)
    
 MIT Licence (MIT)
 Copyright (c) 1997 - 2014 John Mayo-Smith for Federal Digital Coin Corporation
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.

 OpenMultiCryptoATM is based on John Mayo's OpenBitcoinATM.
  
 This application, counts pulses from a Pyramid Technologies Apex 5000
 series bill acceptor and interfaces with the Adafruit 597 TTL serial Mini Thermal 
 Receipt Printer.


  References
  -----------
  John Mayo-Smith: https://github.com/mayosmith
  
  Here's the A2 Micro panel thermal printer --> http://www.adafruit.com/products/597
  
  Here's the bill accceptor --> APEX 5400 on eBay http://bit.ly/MpETED
  
  Peter Kropf: https://github.com/pkropf/greenbacks
  
  Thomas Mayo-Smith:http://www.linkedin.com/pub/thomas-mayo-smith/63/497/a57



 *************************************************************************/


 #include <SoftwareSerial.h>
 #include <Wire.h>
 #include "RTClib.h"
 #include <SPI.h>
 #include <SD.h>

 File logfile; //logfile


 byte cThisChar; //for streaming from SD card
 byte cLastChar; //for streaming from SD card
 char cHexBuf[3]; //for streaming from SD card
 
 const int DOLLAR_PULSE = 4; //pulses per dollar
 const int PULSE_TIMEOUT = 2000; //ms before pulse timeout
 const int MAX_1PEERCOINS = 10; // max $1 ppc per SD card
 const int MAX_5PEERCOINS = 10; // max $5 ppc per SD card
 const int MAX_10PEERCOINS = 10; // max $10 ppc per SD card
 const int MAX_1BITCOINS = 10; //max $1 btc per SD card
 const int MAX_5BITCOINS = 10; //max $5 btc per SD card
 const int MAX_10BITCOINS = 10; //max $10 btc per SD card
 const int HEADER_LEN = 25; //maximum size of bitmap header
 
 #define SET_RTCLOCK      1 // Set to true to set Bitcoin transaction log clock to program compile time.
 #define TEST_MODE        1 // Set to true to not delete private keys (prints the same private key for each dollar).
 
 #define DOUBLE_HEIGHT_MASK (1 << 4) //size of pixels
 #define DOUBLE_WIDTH_MASK  (1 << 5) //size of pixels
 
 RTC_DS1307 RTC; // define the Real Time Clock object

 char LOG_FILE[] = "btclog.txt"; //name of Bitcoin transaction log file
 
 const int chipSelect = 10; //SD module
 
 int printer_RX_Pin = 5;  // This is the green wire
 int printer_TX_Pin = 6;  // This is the yellow wire
 
 char printDensity = 14; // 15; //text darkening
 char printBreakTime = 4; //15; //text darkening

 
 // -- Initialize the printer connection

 SoftwareSerial *printer;
 #define PRINTER_WRITE(b) printer->write(b)
 

 long pulseCount = 0;
 unsigned long pulseTime, lastTime;
 volatile long pulsePerDollar = 4;

void setup(){
  Serial.begin(57600); //baud rate for serial monitor
  attachInterrupt(0, onPulse, RISING); //interupt for Apex bill acceptor pulse detect
  pinMode(2, INPUT); //for Apex bill acceptor pulse detect 
  pinMode(10, OUTPUT); //Slave Select Pin #10 on Uno
  
  if (!SD.begin(chipSelect)) {    
      Serial.println("card failed or not present");
      return;// error("Card failed, or not present");     
  }
  
  
  printer = new SoftwareSerial(printer_RX_Pin, printer_TX_Pin);
  printer->begin(19200);

  //Modify the print speed and heat
  PRINTER_WRITE(27);
  PRINTER_WRITE(55);
  PRINTER_WRITE(7); //Default 64 dots = 8*('7'+1)
  PRINTER_WRITE(255); //Default 80 or 800us
  PRINTER_WRITE(255); //Default 2 or 20us

  //Modify the print density and timeout
  PRINTER_WRITE(18);
  PRINTER_WRITE(35);
  //int printSetting = (printDensity<<4) | printBreakTime;
  int printSetting = (printBreakTime<<5) | printDensity;
  PRINTER_WRITE(printSetting); //Combination of printDensity and printBreakTime

/* For double height text. Disabled to save paper
  PRINTER_WRITE(27);
  PRINTER_WRITE(33);
  PRINTER_WRITE(DOUBLE_HEIGHT_MASK);
  PRINTER_WRITE(DOUBLE_WIDTH_MASK);
*/

  Serial.println();
  Serial.println("Parameters set");
  
   #if SET_RTCLOCK
    // following line sets the RTC to the date & time for Bitcoin Transaction log
     RTC.adjust(DateTime(__DATE__, __TIME__));
   #endif

}

void loop(){
  
  
    if(pulseCount == 0)
     return;
 
    if((millis() - pulseTime) < PULSE_TIMEOUT) 
      return;
 
     if(pulseCount == DOLLAR_PULSE){
       getChoice(1);
     }
     else if(pulseCount == 5 * DOLLAR_PULSE){
       getChoice(5);
     }
     else if(pulseCount == 10 * DOLLAR_PULSE){
       getChoice(10);
     }
     
     //----------------------------------------------------------
     // Add additional currency denomination logic here: $5, $10, $20      
     //----------------------------------------------------------
   
     pulseCount = 0; // reset pulse count
     pulseTime = 0;
  
}

/*****************************************************
onPulse
- read 50ms pulses from Apex Bill Acceptor.
- 4 pulses indicates one dollar accepted

******************************************************/
void onPulse(){
  
int val = digitalRead(2);
pulseTime = millis();

if(val == HIGH)
  pulseCount++;
  
}

/*****************************************************
getNextcoin
- Read next coin QR Code from SD Card
// Should be done

******************************************************/

int getNextcoin(amount, type){
    
  int Number = 0, i = 0;
 // long counter = 0;
 char cBuf, cPrev;
  

       
    Serial.println("card initialized.");
    
    int MAX_COINS = 0
    if(type == PPC){
      if(amount == 1){
        MAX_COINS = MAX_1PEERCOINS
      }
      else if(amount ==5){
        MAX_COINS = MAX_5PEERCOINS
      }
      else{
        MAX_COINS = MAX_10PEERCOINS
    if(type == BTC){
      if(amount == 1){
        MAX_COINS = MAX_1BITCOINS
      }
      else if(amount ==5){
        MAX_COINS = MAX_5BITCOINS
      }
      else{
        MAX_COINS = MAX_10BITCOINS
        
    while(Number<MAX_COINS){
      
         //prepend file name
         String temp = type + String(amount) + "_";
         //add file number
         temp.concat(Number);
         //append extension
         temp.concat(".btc"); 
         
         //char array
         char filename[temp.length()+1];   
         temp.toCharArray(filename, sizeof(filename));
        
         //check if the bitcoin QR code exist on the SD card
         if(SD.exists(filename)){
             Serial.print("file exists: ");
             Serial.println(filename);
             
             //print logo at top of paper
             if(SD.exists("logo.oba")){
               printBitmap("logo.oba"); 
             }  
             
               //----------------------------------------------------------
               // Depends on Exchange Rate 
               // May be removed during volitile Bitcoin market periods
               //----------------------------------------------------------
             
               ///printer->println("Value .002 BTC");

             
               //print QR code off the SD card
               printBitmap(filename); 

               printer->println("Official Bitcoin Currency.");

               printer->println("Keep secure.");

               printer->println("OpenBitcoinATM.org");
               
               printer->println(" ");
               printer->println(" ");
               printer->println(" ");
               printer->println(" ");


          break; //stop looking, coin file found
         }  
          else{
            if (Number >= MAX_COINS -1){
              
                //----------------------------------------------------------
                // Disable bill acceptor when bitcoins run out 
                // pull low on Apex 5400 violet wire
                //----------------------------------------------------------
              
              digitalWrite(disablepin, LOW); //initialize pin up top
            }  
             Serial.print("file does not exist: ");
             Serial.println(filename);        
        }
    //increment coin number
    Number++;
    }
}  

/*****************************************************
printBitmap(char *filename)
- open QR code bitmap from SD card. Bitmap file consists of 
byte array output by OpenBitcoinQRConvert.pde
width of bitmap should be byte aligned -- evenly divisable by 8


******************************************************/
void printBitmap(char *filename){
  int nBytes = 0;
  int iBitmapWidth = 0 ;
  int iBitmapHeight = 0 ;
  File tempFile = SD.open(filename);

        for(int h = 0; h < HEADER_LEN; h++){
        
          cLastChar = cThisChar;
          if(tempFile.available()) cThisChar = tempFile.read(); 
    
              //read width of bitmap
              if(cLastChar == '0' && cThisChar == 'w'){
                if(tempFile.available()) cHexBuf[0] = tempFile.read(); 
                if(tempFile.available()) cHexBuf[1] = tempFile.read(); 
                  cHexBuf[2] = '\0';
                  
                  iBitmapWidth = (byte)strtol(cHexBuf, NULL, 16); 
                  Serial.println("bitmap width");
                  Serial.println(iBitmapWidth);           
              }
    
              //read height of bitmap
              if(cLastChar == '0' && cThisChar == 'h'){
               
                 if(tempFile.available()) cHexBuf[0] = tempFile.read(); 
                 if(tempFile.available()) cHexBuf[1] = tempFile.read(); 
                  cHexBuf[2] = '\0';
                  
                  iBitmapHeight = (byte)strtol(cHexBuf, NULL, 16);
                  Serial.println("bitmap height");
                  Serial.println(iBitmapHeight); 
              }
      }
      
  
      PRINTER_WRITE(0x0a); //line feed

      
      Serial.println("Print bitmap image");
      //set Bitmap mode
      PRINTER_WRITE(18); //DC2 -- Bitmap mode
      PRINTER_WRITE(42); //* -- Bitmap mode
      PRINTER_WRITE(iBitmapHeight); //r
      PRINTER_WRITE((iBitmapWidth+7)/8); //n (round up to next byte boundary
  
  
      //print 
      while(nBytes < (iBitmapHeight * ((iBitmapWidth+7)/8))){ 
        if(tempFile.available()){
            cLastChar = cThisChar;
            cThisChar = tempFile.read(); 
        
                if(cLastChar == '0' && cThisChar == 'x'){
      
                    cHexBuf[0] = tempFile.read(); 
                    cHexBuf[1] = tempFile.read(); 
                    cHexBuf[2] = '\0';
                    Serial.println(cHexBuf);
                    
                    PRINTER_WRITE((byte)strtol(cHexBuf, NULL, 16)); 
                    nBytes++;
                }
        }  
          
      }

       
      PRINTER_WRITE(10); //Paper feed
      Serial.println("Print bitmap done");


  tempFile.close();
    Serial.println("file closed");

    
   #if !TEST_MODE
  //delete the QR code file after it is printed
     SD.remove(filename);
   #endif 
 
 
   // update transaction log file
    //if (! SD.exists(LOG_FILE)) {
      // only open a new file if it doesn't exist
       

    //}
    
  return;
  
}


/*****************************************************
updateLog()
Updates Bitcoin transaction log stored on SD Card
Logfile name = LOG_FILE

******************************************************/
void updateLog(){
  
      DateTime now;
      
      now=RTC.now();
      
      logfile = SD.open(LOG_FILE, FILE_WRITE); 
      logfile.print("Bitcoin Transaction ");
      logfile.print(now.unixtime()); // seconds since 1/1/1970
      logfile.print(",");
      logfile.print(now.year(), DEC);
      logfile.print("/");
      logfile.print(now.month(), DEC);
      logfile.print("/");
      logfile.print(now.day(), DEC);
      logfile.print(" ");
      logfile.print(now.hour(), DEC);
      logfile.print(":");
      logfile.print(now.minute(), DEC);
      logfile.print(":");
      logfile.println(now.second(), DEC);
      logfile.close();
}

/*****************************************************
getChoice()
Is an intermediary step for the user to choose their desired cryptocurrency
//pins need to be configured up top
//Only configured for two different coins at the moment: PPC and BTC

******************************************************/
void getChoice(amount){
      int led = 0;
      int choice = 1;
      char type = ""
      while(choice == 1){
         timer = millis();
         if( millis() - timer >= 1000L){
           if(led == 1){
             led = 0;
             digitalWrite(pinbtc,LOW);
             digitalWrite(pinppc,HIGH);
           }
           else{
             led = 1;
             digitalWrite(pinbtc,HIGH);
             digitalWrite(pinppc,LOW);
           }
         }  
         pin3 = digitalRead(pin3);
         pin4 = digitalRead(pin4);
         if((pin3 || pin4) == HIGH){
           led = 0;
           choice = 0;
           digitalWrite(pinppc, LOW);
           digitalWrite(pinbtc, LOW);
           if( pin3 && pin4 == HIGH){
             getChoice(amount);
           }
           else if(pin3 == HIGH){
             type = "PPC";
             digitalWrite(pinppc,HIGH);
             getnextcoin(amount, type);
           }
           else(pin4 == HIGH) {
             type = "BTC";
             digitalWrite(pinbtc,HIGH);
             getnextcoin(amount, type);
           }
         }
      }
}
