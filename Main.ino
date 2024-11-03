#include <Arduino_USBHostMbed5.h>
#include <DigitalOut.h>
#include <FATFileSystem.h>



USBHostMSD msd;
mbed::FATFileSystem usb("usb");

// mbed::DigitalOut pin5(PC_6, 0);
mbed::DigitalOut otg(PB_8, 1);

void setup() {
  // Initialize pin 50 as an input with an internal pull-up resistor
  pinMode(50, INPUT_PULLUP);  Serial.begin(115200);
  
  pinMode(PA_15, OUTPUT); //enable the USB-A port
  digitalWrite(PA_15, HIGH);
  
  while (!Serial);
  Serial.println("Starting to connect to USB device...");

  msd.connect();
  while (!msd.connect()) {
    //while (!port.connected()) {
    delay(1000);
  }

  Serial.println("Mounting USB device...");
  int err =  usb.mount(&msd);
  if (err) {
    Serial.print("Error mounting USB device ");
    Serial.println(err);
    while (1);
  }
  Serial.print("read done ");
  
  mbed::fs_file_t file;
  struct dirent *ent;
  int dirIndex = 0;
  int res = 0;
  Serial.println("Open /usb/front_wheel_data.txt");

  FILE *f = fopen("/usb/front_wheel_data.txt", "w+");
  if (f == NULL) {
    Serial.print("Failed to open file: ");
    Serial.println(strerror(errno));
    while (1); // Halt execution
  }
}

void loop() {
  // Read the state of pin 50 as data log switch
    int pinState = digitalRead(50);

    // Print the state to the Serial Monitor for debugging
    Serial.print("Pin 50 state: ");
    Serial.println(pinState);

    
}


void write_data(){
  err = fprintf(f, "%d,%ld,%ld,%d,%d\n", i, countsLeft[i], countsRight[i], velocitiesLeft[i], velocitiesRight[i]);
    if (err < 0) {
      Serial.println("Fail :(");
      error("error: %s (%d)\n", strerror(errno), -errno);
    }
}

void close_file(){
  Serial.println("File closing");
  fflush(stdout);
  err = fclose(f);
  if (err < 0) {
    Serial.print("fclose error:");
    Serial.print(strerror(errno));
    Serial.print(" (");
    Serial.print(-errno);
    Serial.print(")");
  } else {
    Serial.println("File closed");
  }
}