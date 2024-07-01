/*
**************************************************************************
This Arduino code is specially designed for a current sensor data logger system 
to measure the short circuit current provided by a solar panel when light shines onto it.
This code requires a competible Arduino Uno, Shield Data logger with DS1307 RTC and an INA219 DC current sensor.


Developed by Tresthon Quah and Dr. Lee Chee Huei, SUTD, 9 May 2024
***************************************************************************
*/

// These are libraries that are used to provide basic functionalities
#include <Wire.h>
#include <INA219_WE.h>
#include <SPI.h> 
#include <SD.h>
#include <RTClib.h>

// These are declarations of external modules used
INA219_WE ina219(0x40);   // Voltage and current sensor
RTC_DS1307 rtc;           // Real-time clock

// These are declarations of global variables
char filename[] = "YYYYMMDD.csv";   // Temporary file name for .csv file
DateTime start_time;                // Temporary start time variable
File file;                          // Temporary file variable for .csv file
float averagedcurrent_mA;           // Temporary variable for averaged values
int counter = 0;                    // Temporary variable to count no. of datapoints
const int LEDPIN = 7;               // Define the GPIO Pin 7 to control the LED7
int frequency = 20;                 // in Seconds (set to be even number), how often to collect current value from the solar panel


// This is the setup function, which is what the Arduino runs once at the start of every iteration
void setup() {

  pinMode(LEDPIN, OUTPUT);
  Serial.begin(9600);       // Open serial communications with a computer
  Wire.begin(); 

  // Checking for INA219 connection
  if(!ina219.init()){
    Serial.println("Error : INA219 not connected!");
    Serial.println("Connect the INA219, then push the reset button");
    digitalWrite(LEDPIN, HIGH); 
    while (1);
  }
  else{
    Serial.println("INA219 detected!");
  }

  // Checking for RTC shield attachment
  if (!rtc.begin()) {
    Serial.println("Error : RTC not detected!");
    Serial.println("Attach the RTC, then push the reset button");
    digitalWrite(LEDPIN, HIGH); 
    while (1);   // This command is an infinite loop. It is the same as for (;;); 
  }
  else{
    Serial.println("RTC detected!");
  }

  // Checking for RTC value
  if (!rtc.isrunning()) {
    Serial.println("Error : RTC lost power!");
    Serial.println("RTC will be set to when this sketch was compiled");
    // When time needs to be set on a new device, or after a power loss, the
    // Following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    digitalWrite(LEDPIN, HIGH); 
    while(1);
  }
  else{
    Serial.println("RTC OK!");
  }
  
/* 
The following rtc.adjust command below sets the date and time to the date and time that the code was compiled from a local computer.
Uncomment the command and upload the whole code to update RTC Data and Time the first time.
Then, comment this line out and upload the code AGAIN (the second time upload) to prevent the RTC resetting to the initial date and time when Arduino is power reset. 

*/
 // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // Checking for SD card to make sure SD card is inserted
  if (!SD.begin(10)) {
    Serial.println("Error : SD card not detected!");
    Serial.println("Attach the SD card, then push the reset button");
    digitalWrite(LEDPIN, HIGH); 
    for (;;);    // This command is an infinite loop. The code will hang there (the rest of the code will not run) until reset button is pressed.
  }
  else{
    Serial.println("SD card detected!");
  }

  // Generating filename (Based on current RTC date) 
  start_time = rtc.now();
  filename[0] = '0' + (start_time.year() / 1000);  // start_time is an integer. start_time.year()/1000, as integer gives the first digit of the year.
  filename[1] = '0' + (start_time.year() / 100) % 10;  // % is the modulo operation, which returns the remainder of an integer division
  filename[2] = '0' + (start_time.year() / 10) % 10;
  filename[3] = '0' + (start_time.year() % 10);
  filename[4] = '0' + (start_time.month() / 10) % 10;
  filename[5] = '0' + (start_time.month() % 10);
  filename[6] = '0' + (start_time.day() / 10) % 10;
  filename[7] = '0' + (start_time.day() % 10); 

  // Opening file and listing the headers of the .csv file
  file = SD.open(filename, FILE_WRITE);
  if (file.size() == 0) {
    file.println("Time, Current (mA)");
  }
  file.close();
  Serial.println("SD card OK!");

  // End of setup function
  Serial.println("All checks OK!");
  Serial.print("Data will be stored into ");
  Serial.println(filename);
  Serial.println();
}

// This is the loop function, which is what the Arduino will be constantly doing while running
void loop() {
  
  // checking the current sensor connection in every iteration/loop.
  if(!ina219.init()) {
    Serial.println("Error : INA219 not connected! Connect the INA219, then push the reset button");
    digitalWrite(LEDPIN, HIGH); 
    while (1);    // This command is an infinite loop. The code will hang there until reset button is pressed.
  }

  // checking the SD card in every iteration/loop.
  if (!SD.begin(10)) {
    Serial.println("Error : SD card not detected! Attach the SD card, then push the reset button");
    digitalWrite(LEDPIN, HIGH); 
    while (1);   
  }

  // Open file and store data
  file = SD.open(filename, FILE_WRITE);
  ReadData();
  file.close();

  /*  Flashing the on-board LED as the indicator that the code in loop() is running 
      and there are data being stored in SD card.
  */
  for (int i = 1; i <= frequency/2; i++) {
    digitalWrite(LEDPIN, HIGH);     // turn the LED on (HIGH is the voltage level). GPIO7 will give out 5V.
    delay((frequency/frequency)*1000);  // delay 1 second for LED to be on for 1 second
    digitalWrite(LEDPIN, LOW);      // turn the LED off (LOW is the voltage level). GPIO7 will be 0V.
    delay((frequency/frequency)*1000);   // LED to be off for 1 second
  }
}

// This function reads the current of the solar panel
void ReadData() {
  
  // Get current reading from the INA219 module
  float current_mA = ina219.getCurrent_mA();
  StoreData(current_mA);

  /*  Using the following commands to measure bus voltage and power if needed. 
      StoreData function would need to be modified accordingly to store additional data in SD card.
  */
  // float busVoltage_V = ina219.getBusVoltage_V();
  // float power_mW = ina219.getBusPower();

}

// This function stores the data values into the SD card
void StoreData(float currentvalue) {

  // Generate current timestamp
  DateTime now = rtc.now();
  String currtime = String(now.hour());
  currtime.concat(":");
  currtime.concat(String(now.minute()));
  currtime.concat(":");
  currtime.concat(String(now.second()));

  // Print out the time and current value in the serial monitor for checking 
  Serial.print("At "); Serial.println(currtime);
  Serial.print("Current[mA]: "); Serial.println(currentvalue);

  // Store recorded time on SD card
  file.print(currtime);
  file.print(",");
  
  // Store current value on SD card
  file.print(currentvalue);
  file.print(",");

  Serial.println("Data stored!");
  Serial.println();

  // Prepare for next line of stored readings
  file.println();
}
