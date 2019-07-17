  // include the library code:
  
  #include <arduino.h>
  #include <LiquidCrystal.h>
  #include <Wire.h>
  #include "IRLibAll.h"
  #include "remote_code.h"
  #include "PT2258.h"
  
  /*The circuit:
  * LCD RS pin to digital pin 12
  * LCD Enable pin to digital pin 11
  * LCD D4 pin to digital pin 6
  * LCD D5 pin to digital pin 5
  * LCD D6 pin to digital pin 4
  * LCD D7 pin to digital pin 3
  * LCD R/W pin to ground
     */

 #define outputA 6
 #define outputB 7
 
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /*2258 library code */

  unsigned char channell_address01[6] = 
  {CHANNEL1_VOLUME_STEP_01, CHANNEL2_VOLUME_STEP_01, CHANNEL3_VOLUME_STEP_01, 
   CHANNEL4_VOLUME_STEP_01, CHANNEL5_VOLUME_STEP_01, CHANNEL6_VOLUME_STEP_01};

unsigned char channell_address10[6] = 
  {CHANNEL1_VOLUME_STEP_10, CHANNEL2_VOLUME_STEP_10, CHANNEL3_VOLUME_STEP_10, 
   CHANNEL4_VOLUME_STEP_10, CHANNEL5_VOLUME_STEP_10, CHANNEL6_VOLUME_STEP_10};

// helper method
unsigned char PT2258::HEX2BCD (unsigned char x)
{
    unsigned char y;
    y = (x / 10) << 4;
    y = y | (x % 10);
    return (y);
}
   
// helper method
int PT2258::writeI2CChar(unsigned char c)   
{   
//  shift address to right - Wire library always uses 7 bit addressing
    Wire.beginTransmission(0x88 >> 1); // transmit to device 0x88, PT2258
    Wire.write(c);   
    int rtnCode = Wire.endTransmission(); // stop transmitting
    return rtnCode;
} 

// initialize PT2258
int PT2258::init(void)   
{   
    delay(300); // in case this is first time - I2C bus not ready for this long on power on with 10uF cref

    unsigned char masterVolumeValue = 0x05;    //master volume 0dB
    unsigned char channelVolume = 0x05;   // channel volume 0dB
        
    // initialize device
    writeI2CChar(SYSTEM_RESET);

    // set channell volumes to zero
    for(int chno=0; chno<6; chno++)
    {
      Wire.beginTransmission(0x88 >> 1); // transmit to device 0x88, PT2258
      Wire.write(channell_address01[chno] | (HEX2BCD(channelVolume)   &  0x0f));   
      Wire.write(channell_address10[chno] | ((HEX2BCD(channelVolume)  &  0xf0)>>4));   
      Wire.endTransmission();       // stop transmitting
    }
    
    // set the master voume
    Wire.beginTransmission(0x88 >> 1); // transmit to device 0x88, PT2258
    Wire.write(MASTER_VOLUME_1STEP | (HEX2BCD(masterVolumeValue)  &  0x0f));   
    Wire.write(MASTER_VOLUME_10STEP | ((HEX2BCD(masterVolumeValue)  &  0xf0)>>4));   
    Wire.endTransmission();       // stop transmitting

    // set mute off
    return writeI2CChar(MUTE | 0x00);
}   

// Set mute: 1 -> mute on, 0 -> mute off
void PT2258::setMute(char in_mute)   
{   
  writeI2CChar(MUTE | in_mute);
}   

// Set channel volume, attenuation range : 0 to 79dB
void PT2258::setChannelVolume(unsigned char chvol, char chno)
{   
  Wire.beginTransmission(0x88 >> 1); // transmit to device 0x88, PT2258
  Wire.write(channell_address01[chno] | (HEX2BCD(chvol)   &  0x0f));   
  Wire.write(channell_address10[chno] | ((HEX2BCD(chvol)  &  0xf0)>>4));   
  Wire.endTransmission();       // stop transmitting
}

// Set master volume, attenuation range : 0 to 79dB
void PT2258::setMasterVolume(unsigned char mvol)   
{   
  Wire.beginTransmission(0x88 >> 1); // transmit to device 0x88, PT2258
  Wire.write(MASTER_VOLUME_1STEP  | (HEX2BCD(mvol)   &  0x0f));   
  Wire.write(MASTER_VOLUME_10STEP | ((HEX2BCD(mvol)  &  0xf0)>>4));   
  Wire.endTransmission();       // stop transmitting
}
  /* initialize the library with the numbers of the interface pins */
  LiquidCrystal lcd(12, 11, 6, 5, 4, 3);
  /*Create a receiver object to listen on pin 2 */
  IRrecvPCI myReceiver(2);
  void remote_update(uint32_t val);
  char chk_min_max_volume(char val_chk);
  
  /*  Create a decoder object  */
  IRdecode myDecoder;  
  uint32_t decode_val;
  PT2258 audio;

  ///////////////////////////
  /*Rotary variables*/
  int counter = 0; 
 int aState;
 int aLastState; 

  /////////////////////////////
  void setup() 
  {
   int i2c_wrt_rst=0;
   pinMode (outputA,INPUT); /*rot*/
   pinMode (outputB,INPUT); /*rot*/
  Serial.begin(9600);
  
  delay(2000); while (!Serial); //delay for Leonardo
  myReceiver.enableIRIn(); // Start the receiver
  Serial.println(F("Ready to receive IR signals"));
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("My IR code");
  delay(500);delay(500);
  Wire.begin(); // join i2c bus
  i2c_wrt_rst=audio.init();
  //lcd.setCursor(0, 1);
 // lcd.print("Return=         ");
  //lcd.print(i2c_wrt_rst);
  //delay(500);
  //audio.setMute(0);

  aLastState = digitalRead(outputA); /*rot*/
  }
  
  void loop() 
  {
    
  //Continue looping until you get a complete signal received
  if (myReceiver.getResults())
  {
    myDecoder.decode();           //Decode it
    myDecoder.dumpResults(true);  //Now print results. Use false for less detail
    decode_val=myDecoder.value;
    myReceiver.enableIRIn();      //Restart receiver
    remote_update(decode_val);
    // delay(50); 
  }


  /*Rotary control*/
////////////////////////////////////////////////////////////////////////////////////////

 aState = digitalRead(outputA); // Reads the "current" state of the outputA
   // If the previous and the current state of the outputA are different, that means a Pulse has occured
   if (aState != aLastState){     
     // If the outputB state is different to the outputA state, that means the encoder is rotating clockwise
     if (digitalRead(outputB) != aState) { 
       counter ++;
       remote_update(MAS_PLUS);
     } else 
     {
       counter --;
       remote_update(MAS_MIN);
     }
     Serial.print("Position: ");
     Serial.println(counter);
   } 
   aLastState = aState; // Updates the previous state of the outputA with the current state
///////////////////////////////////////////////////////////////////////////////////////

  
  }
 /////////////////////////////////////////////////////////////////////////////////////////////////
   /*Remote deoder part */  
  void remote_update(uint32_t val)
  {

    /////////////////////////////////////////////
    lcd.setCursor(0, 0);
    lcd.print("2258 Audio Test ");
    //lcd.print("val: ");
    //lcd.setCursor(5, 0);
   // lcd.print(val,HEX);
  ////////////////////////////////////////////////
    
    static unsigned char mas_vol=10;
    static unsigned char wof_vol=0;
    static unsigned char frt_vol=5;
    static unsigned char rer_vol=5;
    static unsigned char cen_vol=5;
    static unsigned char ch1_vol=5;
    static unsigned char ch2_vol=5;
    static unsigned char muteButton=0;
  switch (val)
  {
    case STANDBY:
          //lcd.clear();
          lcd.setCursor(0, 1);
          lcd.print("standby/mute    ");
          if(muteButton==0)
          {
           audio.setMute(1);
           muteButton++;
           break;
          }
          else
          {
            audio.setMute(0);
            muteButton--;
            break;
          }
          break;
    case INPUT_V:
          //lcd.clear();
          lcd.setCursor(0, 1);
          lcd.print("input           ");
          break;
    case RESET:
          lcd.setCursor(0, 1);
          lcd.print("reset           ");
          break;
    case PRO_LOGIC:
          lcd.setCursor(0, 1);
          lcd.print("prologic        ");
          break;
   case SAVE:
          lcd.setCursor(0, 1);
          lcd.print("save            ");
          break;
    case MAS_MIN:
          lcd.setCursor(0, 1);
          mas_vol=mas_vol-5;
          mas_vol=chk_min_max_volume(mas_vol); 
          lcd.print("mas--           ");
          lcd.setCursor(7, 1);
          lcd.print(mas_vol,DEC);
          audio.setMasterVolume(mas_vol);
          break;
    case MAS_PLUS:
           lcd.setCursor(0, 1);
           mas_vol=mas_vol+5;
           mas_vol=chk_min_max_volume(mas_vol); 
           lcd.print("mas++           ");
           lcd.setCursor(7, 1);
           lcd.print(mas_vol,DEC);
          audio.setMasterVolume(mas_vol);
          break;
 case WOOF_MIN:
          lcd.setCursor(0, 1);
          wof_vol=wof_vol-5;
          wof_vol=chk_min_max_volume(wof_vol); 
          lcd.print("WoF--           ");
          lcd.setCursor(7, 1);
          lcd.print(wof_vol,DEC);
          audio.setChannelVolume(wof_vol,1);
          break;  
 case WOOF_PLUS:
          lcd.setCursor(0, 1);
          wof_vol=wof_vol+5;
          wof_vol=chk_min_max_volume(wof_vol);  
          lcd.print("WoF++           ");
          lcd.setCursor(7, 1);
          lcd.print(wof_vol,DEC);
          audio.setChannelVolume(wof_vol,1);
          break;   
case FRT_PLUS:
          lcd.setCursor(0, 1);
          frt_vol=frt_vol+5;
          frt_vol=chk_min_max_volume(frt_vol);  
          lcd.print("FRT++          ");
          lcd.setCursor(7, 1);
          lcd.print(frt_vol,DEC);
          audio.setChannelVolume(frt_vol,2);
          break; 
case FRT_MIN:
          lcd.setCursor(0, 1);
          frt_vol=frt_vol-5;
          frt_vol=chk_min_max_volume(frt_vol);  
          lcd.print("FRT--          ");
          lcd.setCursor(7, 1);
          lcd.print(frt_vol,DEC);
          audio.setChannelVolume(frt_vol,2);
          break; 
case RER_PLUS:
          lcd.setCursor(0, 1);
          rer_vol=rer_vol+5;
          rer_vol=chk_min_max_volume(rer_vol);  
          lcd.print("RER++          ");
          lcd.setCursor(7, 1);
          lcd.print(rer_vol,DEC);
          audio.setChannelVolume(rer_vol,3);
          break;   
case RER_MIN:
          lcd.setCursor(0, 1);
          rer_vol=rer_vol-5;
          rer_vol=chk_min_max_volume(rer_vol);  
          lcd.print("RER--          ");
          lcd.setCursor(7, 1);
          lcd.print(rer_vol,DEC);
          audio.setChannelVolume(rer_vol,3);
          break;
case CEN_PLUS:
          lcd.setCursor(0, 1);
          cen_vol=cen_vol+5;
          cen_vol=chk_min_max_volume(cen_vol);  
          lcd.print("CEN++          ");
          lcd.setCursor(7, 1);
          lcd.print(cen_vol,DEC);
          audio.setChannelVolume(cen_vol,4);
          break;  
case CEN_MIN:
          lcd.setCursor(0, 1);
          cen_vol=cen_vol-5;
          cen_vol=chk_min_max_volume(cen_vol);  
          lcd.print("CEN--          ");
          lcd.setCursor(7, 1);
          lcd.print(cen_vol,DEC);
          audio.setChannelVolume(cen_vol,4);
          break; 
case CH_PLUS:
          lcd.setCursor(0, 1);
          ch1_vol=ch1_vol+5;
          ch1_vol=chk_min_max_volume(ch1_vol);  
          lcd.print("CH1++          ");
          lcd.setCursor(7, 1);
          lcd.print(ch1_vol,DEC);
          audio.setChannelVolume(ch1_vol,5);
          break;
case CH_MIN:
          lcd.setCursor(0, 1);
          ch1_vol=ch1_vol-5;
          ch1_vol=chk_min_max_volume(ch1_vol);  
          lcd.print("CH1--          ");
          lcd.setCursor(7, 1);
          lcd.print(ch1_vol,DEC);
          audio.setChannelVolume(ch1_vol,5);
          break;
 case TUNE_PLUS:
          lcd.setCursor(0, 1);
          ch2_vol=ch2_vol+5;
          ch2_vol=chk_min_max_volume(ch2_vol);  
          lcd.print("CH2++          ");
          lcd.setCursor(7, 1);
          lcd.print(ch2_vol,DEC);
          audio.setChannelVolume(ch2_vol,6);
          break;
 case TUNE_MIN:
          lcd.setCursor(0, 1);
          ch2_vol=ch2_vol-5;
          ch2_vol=chk_min_max_volume(ch2_vol);  
          lcd.print("CH2--          ");
          lcd.setCursor(7, 1);
          lcd.print(ch2_vol,DEC);
          audio.setChannelVolume(ch2_vol,6);
          break;
          
   default:
          lcd.setCursor(0, 1);
          lcd.print("not pressed     ");
          break;
      }
     }
 ////////////////////////////////////////////////////////////////////////////////////////////////

char chk_min_max_volume(char val_chk)
  {
      char vol;
      if(val_chk<6)
          val_chk=5;
      else if(val_chk>79)
          val_chk=79;
      else
         val_chk=val_chk;
    
       return val_chk;
  }
  
