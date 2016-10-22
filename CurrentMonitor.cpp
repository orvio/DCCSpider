/**********************************************************************

CurrentMonitor.cpp
COPYRIGHT (c) 2013-2016 Gregg E. Berman

Part of DCC++ BASE STATION for the Arduino

**********************************************************************/

//#include "DCCpp_Uno.h"
#include "CurrentMonitor.h"
//#include "Comm.h"

///////////////////////////////////////////////////////////////////////////////

CurrentMonitor::CurrentMonitor(int pin, byte signalEnablePin){
    this->pin=pin;
    this->signalEnablePin = signalEnablePin;
    current=0;
  } // CurrentMonitor::CurrentMonitor
  
boolean CurrentMonitor::checkTime(){
  if(millis()-sampleTime<CURRENT_SAMPLE_TIME)            // no need to check current yet
    return(false);
  sampleTime=millis();                                   // note millis() uses TIMER-0.  For UNO, we change the scale on Timer-0.  For MEGA we do not.  This means CURENT_SAMPLE_TIME is different for UNO then MEGA
  return(true);  
} // CurrentMonitor::checkTime
  
void CurrentMonitor::check(){
  current=analogRead(pin)*CURRENT_SAMPLE_SMOOTHING+current*(1.0-CURRENT_SAMPLE_SMOOTHING);        // compute new exponentially-smoothed current
  if(current>CURRENT_SAMPLE_MAX && digitalRead(this->signalEnablePin)==HIGH){                    // current overload and Prog Signal is on (or could have checked Main Signal, since both are always on or off together)
    digitalWrite(this->signalEnablePin,LOW);
    //INTERFACE.print(msg);                                                                            // print corresponding error message
  }    
} // CurrentMonitor::check  

long int CurrentMonitor::sampleTime=0;

