//#include <SimpleTimer.h>
//SimpleTimer timer;

int sensorInterrupt = 0;  // 0 = digital pin 2
int sensorPin       = 2;
int pumpPin = 3;
unsigned long c=0;
unsigned long czas=10000; //minuty*60000 czas przerwy
unsigned long praca=5000; // minuty*60000 praca pompy


float calibrationFactor = 6.6;

volatile long pulseCount =0;  

float flowRate;

unsigned long oldTime;
unsigned long oldTime1;

void pumpcontrol(){
 
  if (flowRate >= 0.4 && c==0) {
    digitalWrite(pumpPin, HIGH);   // turn off pump //s*
     Serial.println("Pompa ON");
      c++;
     oldTime1=millis();
     if((millis()-oldTime1)>= praca){
      oldTime2=oldTime1
          digitalWrite(pumpPin, LOW);
         Serial.println("Pompa OFF"); 
     }
    
  }
  if (flowRate  < 0.4 && c==0) {
    digitalWrite(pumpPin, LOW);   // turn on pump //s*
    Serial.println("Pompa OFF");
  
}
//attachInterrupt(pumpPin, pumpcontrol, CHANGE);


}

void flow(){
   if((millis() - oldTime) > 1000)    // Only process counters once per second
  { 
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(sensorInterrupt);
        
    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    
    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();
    
      
   
    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("\n");       // Print tab space

    
    

    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
    
    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  }
}

void zerowanie(){
  
  if((millis()-oldTime1)>= czas){
    oldTime1=millis();
  c=0;
  }
}

void setup()
{
  
  // Initialize a serial connection for reporting values to the host
  Serial.begin(9600);
   
  // Set up the status LED line as an output
 // pinMode(statusLed, OUTPUT);
 // digitalWrite(statusLed, HIGH);  // We have an active-low LED attached
  
  pinMode(sensorPin, INPUT);
    digitalWrite(sensorPin, LOW);

  
  pinMode(pumpPin, OUTPUT);
digitalWrite(pumpPin, LOW);

  pulseCount        = 0;
  flowRate          = 0.0;
  oldTime           = 0;
  c=0;

attachInterrupt(sensorInterrupt, pulseCounter, FALLING);

//attachInterrupt(pumpPin, pumpcontrol, CHANGE);
//timer.setInterval(1000L, pumpcontrol );
//timer.setInterval(czas*60000L,zerowanie);
}



/**
 * Main program loop
 */
void loop()
{
        flow(); 
        pumpcontrol();
        zerowanie();
       // timer.run();
}
/*
Insterrupt Service Routine
 */
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}
