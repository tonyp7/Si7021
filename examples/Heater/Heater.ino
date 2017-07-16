#include "Si7021.h"

Si7021 si7021;


void setup() {
  
  uint64_t serialNumber = 0ULL;

  Serial.begin(115200);
  si7021.begin();

}

void printInfo(){
  Serial.print("Humidity: ");
  Serial.print(si7021.measureHumidity());
  Serial.print("% - Temperature: ");
  Serial.print(si7021.getTemperatureFromPreviousHumidityMeasurement());
  Serial.println("C");
}

void loop() {

  //turn the heater on for 10s
  //you can set the heater from any value from 1 to 15 but be careful of current draw.
  //Max value (=0x0F) draws 95ma at 3.3v.
  //Heater is good to unfog the sensor and should be used in a high humidity environment.
  //Don't forget to turn it off!
  Serial.println("Turning heater ON");
  si7021.setHeater(true, 0x04); //draws 28mA at 3.3V
  for(int i=0; i<20; i++){
	printInfo();
    delay(500);
  }
  
  Serial.println("Turning heater OFF");
  si7021.setHeater(false);
  while(1){
    printInfo();
    delay(500);
  }

  
  


}