/*
  Si7021.cpp
  Very simple class to interface to a Si7021 temperature and humidity sensor.

  This example code is licensed under CC BY 4.0.
  Please see https://creativecommons.org/licenses/by/4.0/

  modified 15th July 2017
  by Tony Pottier
  https://idyl.io
  
*/

#include <stdint.h>
#include <arduino.h>
#include <Wire.h>
#include "Si7021.h"

Si7021::Si7021(){};

void Si7021::begin(){
  //init Arduino I2C lib
  Wire.begin();
}


/// <summary>Write a specific instruction to the sensor and return the result</summary>
/// <param name="instr">The instruction</param>
/// <param name="returnSize">Number of bytes expected. Due to Arduino library being retarded this must be a signed int</param>  
/// <returns>Read result</returns>
uint16_t Si7021::readSensor(const uint8_t instr, const int8_t returnSize){
	
	//timeout counter
	uint8_t timeout = 0;
	
	//send the instruction
	Wire.beginTransmission(SI7021_ADDRESS);
	Wire.write(instr);
	Wire.endTransmission();
	
	//according to the spreadsheet the longest time to wait is 14bit resolution humidity + temp, which is 12 + 10.8ms.
	//25 is therefore a safe value for all instructions
	delay(25);
	
	//wait for data to be ready
	Wire.requestFrom(SI7021_ADDRESS, returnSize);
	while (Wire.available() < returnSize){
		delay(1);
		timeout++;
		if(timeout > SI7021_READ_TIMEOUT){
			//later down we force the last 2 bit to be 0.
			//Therefore 1 is not a possible value and can be caught as an error if needed
			return 1;
		}
	}
	
	uint16_t msb = Wire.read();
	uint8_t lsb = Wire.read();
	
	if(returnSize >= 3){
		Wire.read(); //third byte is the checksum
	}
	
	//a humidity measurement will always return XXXXXX10 in the LSB field.
	//Clear the last 2 bits of lsb. Little quirk of the sensor!
	lsb &= 0xFC;
	
	return (uint16_t)((msb << 8) | lsb);
}

/// <summary>
///		Measure the temperature. Since reading humidity also forces a temperature measurement, you shouldn't use this function unless you just want the temperature
/// 	<seealso cref="Si7021::getTemperatureFromPreviousHumidityMeasurement"/>  
/// </summary>
/// <returns>Temperature, in Celcius</returns>
float Si7021::measureTemperature(){
	uint16_t dword = readSensor(SI7021_MEASURE_TEMPERATURE_NO_HOLD_MASTER_MODE, (uint8_t)3);
	return 175.25f * dword / 65536.0f - 46.85f;
}

/// <summary>
///		Measure the temperature. Since reading humidity also forces a temperature measurement, you shouldn't use this function unless you just want the temperature
/// 	<seealso cref="Si7021::getTemperatureFromPreviousHumidityMeasurementF"/>  
/// </summary>
/// <returns>Temperature, in Fahrenheit </returns>
float Si7021::measureTemperatureF(){
	return this->measureTemperature() * 1.8f + 32.0f;
}

/// <summary>
///		Measure the humidy.
/// </summary>
/// <returns>Relative Humidity, in percent</returns>
float Si7021::measureHumidity(){
	uint16_t dword = readSensor(SI7021_MEASURE_HUMIDIY_NO_HOLD_MASTER_MODE, (uint8_t)3);
	return 125.0f * dword / 65536.0f - 6.0f;
}


/// <summary>
///		Each time a relative humidity measurement is made a temperature measurement is also made for the purposes of
///		temperature compensation of the relative humidity measurement. If the temperature value is required, it can be
///		read using this function; this avoids having to perform a second temperature measurement. 
/// </summary>
/// <returns>Temperature, in Celcius</returns>
float Si7021::getTemperatureFromPreviousHumidityMeasurement(){
	uint16_t dword = readSensor(SI7021_READ_TEMPERATURE_FROM_PREVIOUS_RH_MEASUREMENT, (uint8_t)2);
	return 175.25f * dword / 65536.0f - 46.85f;
}

/// <summary>
///		Each time a relative humidity measurement is made a temperature measurement is also made for the purposes of
///		temperature compensation of the relative humidity measurement. If the temperature value is required, it can be
///		read using this function; this avoids having to perform a second temperature measurement. 
/// </summary>
/// <returns>Temperature, in Fahrenheit</returns>
float Si7021::getTemperatureFromPreviousHumidityMeasurementF(){
	return getTemperatureFromPreviousHumidityMeasurementF() * 1.8f + 32.0f;
}

/// <summary>
///		Soft reset of the chip.
/// </summary>
void Si7021::reset(){
	Wire.beginTransmission(SI7021_ADDRESS);
	Wire.write(SI7021_RESET);
	Wire.endTransmission();
	delay(15); //datasheet specifies device takes up to 15ms (5ms typical) before going back live
}

/// <summary>
///		Read the 64 bit serial number of the Si7021 sensor
///		Because 64 bit is expensive this should be used for debugging purposes only.
/// </summary>
/// <returns>Serial number</returns>
uint64_t Si7021::getSerialNumber(){
	
	uint64_t serialNo = 0ULL;
	uint8_t buffer = 0x00;

	
	Wire.beginTransmission(SI7021_ADDRESS);
	Wire.write(0xFA);
	Wire.write(0X0F);
	Wire.endTransmission();

	//4 byte + CRC for each
	Wire.requestFrom(SI7021_ADDRESS, 8);
	while (Wire.available() >= 2){
		buffer = Wire.read();
		serialNo = (serialNo << 8) | buffer;
		Wire.read(); //discard checksum
	}
	
	Wire.beginTransmission(SI7021_ADDRESS);
	Wire.write(0xFC);
	Wire.write(0xC9);
	Wire.endTransmission();

	//4 byte + CRC for each
	Wire.requestFrom(SI7021_ADDRESS, 8);
	while (Wire.available() >= 2){
		buffer = Wire.read();
		serialNo = (serialNo << 8) | buffer;
		Wire.read(); //discard checksum
	}
	
	return serialNo;
	
}

/// <summary>
///		Get the firmware revision. 0xFF = Firmware version 1.0 0x20 = Firmware version 2.0
///	</summary>
/// <returns>Firmware version</returns>
uint8_t Si7021::getFirmwareVersion() {
	uint8_t buffer = 0x00;

	Wire.beginTransmission(SI7021_ADDRESS);
	Wire.write(0x84);
	Wire.write(0xB8);
	Wire.endTransmission();

	//1 byte only: no checksum
	Wire.requestFrom(SI7021_ADDRESS, 1);
	while (Wire.available()) {
		buffer = Wire.read();
	}

	return buffer;
}

/// <summary>
///		Turn on/off the heater.
/// <param name="on">Set to TRUE for on, FALSE for off</param>
/// <param name="power">Value between 0 and 15. Strength of the heater. WARNING: power value of 15 uses up to 95mA at 3.3V. Consult the documentation</param>
///	</summary>
void Si7021::setHeater(bool on, uint8_t power)
{
	uint8_t reg = 0x00;

	if (on) {

		//filter user input and write heat control (from 0x00 to 0x0F)
		reg = this->readRegister(SI7021_READ_HEATER_CONTROL_REGISTER);
		reg |= (power & 0x0F);
		this->writeRegister(SI7021_WRITE_HEATER_CONTROL_REGISTER, reg);

		//turn on heater by turning on bit HTRE which is the 3rd bit hence the 0x04 (0b100) mask 
		reg = 0x3A; //0011 1010 is the default value of this register.
		reg = this->readRegister(SI7021_READ_USER_REGISTER);
		reg |= 0x04;
		this->writeRegister(SI7021_WRITE_USER_REGISTER, reg);

	}
	else {
		//turn off heater by turning off bit HTRE which is the 3rd bit hence the 0xFB (1111 1011) mask
		reg = this->readRegister(SI7021_READ_USER_REGISTER);
		reg &= 0xFB;
		this->writeRegister(SI7021_WRITE_USER_REGISTER, reg);
	}
}


uint8_t Si7021::readRegister(uint8_t registerAddress)
{
	uint8_t buffer;

	Wire.beginTransmission(SI7021_ADDRESS);
	Wire.write(registerAddress);
	Wire.endTransmission();

	//1 byte only: no checksum
	Wire.requestFrom(SI7021_ADDRESS, 1);
	while (Wire.available()) {
		buffer = Wire.read();
	}
	return buffer;
}

void Si7021::writeRegister(uint8_t registerAddress, uint8_t value) {
	Wire.beginTransmission(SI7021_ADDRESS);
	Wire.write(registerAddress);
	Wire.write(value);
	Wire.endTransmission();
}


/// <summary>
///		Set the resolution of sensor.
/// </summary>
/// <param name="resolution">Resolution can be: 0 (12 bit RH, 14 bit Temperature), 1 (8 bit RH, 12 Bit Temperature), 2 (10 bit RH, 13 bit Temperature) or 3 (11 bit RH, 11 bit Temperature)</param>
void Si7021::setSensorResolution(uint8_t resolution) {
	//D7; D0 RES[1:0] Measurement Resolution
	//D7;D0 :	RH		Temp 
	// 00	:	12 bit	14 bit 
	// 01	:	8 bit	12 bit 
	// 10	:	10 bit	13 bit 
	// 11	:	11 bit	11 bit
	//Why they chose to have the MSB and LSB of user register, making it awkward
	//to set this resolution? We may never know.

	uint8_t reg = 0x3A; //0011 1010 is the default value of this register
	reg = this->readRegister(SI7021_READ_USER_REGISTER);
	
	//zero off D7 and D0 of the register
	reg &= 0x7E;

	//apply resolution
	switch (resolution) {
		case 0x01:
			reg |= 0x01;
			break;
		case 0x02:
			reg |= 0x80;
			break;
		case 0x03:
			reg |= 0x81;
		default:
			break;
	}

	this->writeRegister(SI7021_WRITE_USER_REGISTER, reg);


}