#include <Arduino_USBHostMbed5.h>
#include <DigitalOut.h>
#include <FATFileSystem.h>

USBHostMSD msd;
mbed::FATFileSystem usb("usb");

// mbed::DigitalOut pin5(PC_6, 0);
mbed::DigitalOut otg(PB_8, 1);

void setup() {
  Serial.begin(115200);
  
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
  Serial.println("Open /usb/numbers.txt");

  FILE *f = fopen("/usb/numbers.txt", "w+");
  if (f == NULL) {
    Serial.print("Failed to open file: ");
    Serial.println(strerror(errno));
    while (1); // Halt execution
}

  for (int i = 0; i < 10; i++) {
    Serial.print("Writing numbers (");
    Serial.print(i);
    Serial.println("/10)");
    fflush(stdout);
    err = fprintf(f, "%d\n", i);
    if (err < 0) {
      Serial.println("Fail :(");
      error("error: %s (%d)\n", strerror(errno), -errno);
    }
  }

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

void loop() {
  // put your main code here, to run repeatedly:

}