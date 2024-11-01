/*

ENGS 89/90
Testing - Arduino Front Sensor
11/1/2024

*/

#include <Arduino.h>
#include <SPI.h>

// -------------------- Function Declarations --------------------
void write_data();
void close_file();

// -------------------- EncoderManager Class Definition --------------------
class EncoderManager {
public:
    /**
     * Constructor to initialize the encoder with a specific chip select pin.
     * @param encoderPin The Arduino pin used for Chip Select (CS) for this encoder.
     */
    EncoderManager(int encoderPin);

    /**
     * Initializes the encoder by setting up the CS pin and configuring the LS7366.
     */
    void begin();

    /**
     * Retrieves the current encoder count.
     * @return The encoder count as a long integer.
     */
    long getEncoderValue();

    /**
     * Resets the encoder count to zero.
     */
    void reset();

private:
    int _encoderPin; // Chip Select pin for the encoder

    /**
     * Selects the encoder by setting the CS pin LOW.
     */
    void selectEncoder();

    /**
     * Deselects the encoder by setting the CS pin HIGH.
     */
    void deselectEncoder();
};

// -------------------- EncoderManager Class Implementation --------------------

EncoderManager::EncoderManager(int encoderPin) : _encoderPin(encoderPin) {}

void EncoderManager::begin() {
    pinMode(_encoderPin, OUTPUT);
    digitalWrite(_encoderPin, HIGH); // Deselect encoder
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV16); // Adjust as needed
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);

    // Initialize LS7366 Control Register 0
    selectEncoder();
    SPI.transfer(0x88); // Write to Control Register 0 (example command)
    SPI.transfer(0x03); // Configuration data (example value)
    deselectEncoder();

    // Optionally, reset the encoder count
    reset();
}

long EncoderManager::getEncoderValue() {
    unsigned int count1, count2, count3, count4;
    long result;

    selectEncoder();
    SPI.transfer(0x60); // Read Counter Register (example command)
    count1 = SPI.transfer(0x00); // Read highest byte
    count2 = SPI.transfer(0x00);
    count3 = SPI.transfer(0x00);
    count4 = SPI.transfer(0x00); // Read lowest byte
    deselectEncoder();

    result = ((long)count1 << 24) | ((long)count2 << 16) | ((long)count3 << 8) | (long)count4;
    return result;
}

void EncoderManager::reset() {
    selectEncoder();
    SPI.transfer(0x20); // Clear Counter Register (example command)
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    deselectEncoder();
}

void EncoderManager::selectEncoder() {
    digitalWrite(_encoderPin, LOW);
}

void EncoderManager::deselectEncoder() {
    digitalWrite(_encoderPin, HIGH);
}

// -------------------- Global Variables and Instances --------------------

#define INTERVAL 10 //10ms
// Define text file pointer
FILE *f;

// Define encoder chip select (CS) pins
const int ENCODER_LEFT_CS_PIN = 8;
const int ENCODER_RIGHT_CS_PIN = 9;


// Create EncoderManager instances for left and right encoders
EncoderManager encoderLeft(ENCODER_LEFT_CS_PIN);
EncoderManager encoderRight(ENCODER_RIGHT_CS_PIN);

// Variables to store previous counts and time for velocity calculation
long prevCountLeft = 0;
long prevCountRight = 0;
unsigned long prevTime = 0;

// Variables to store calculated velocities
float leftVelocity = 0.0;
float rightVelocity = 0.0;

// Variables to store velocities as integers (multiplied by 1000)
int leftVelocityInt = 0;
int rightVelocityInt = 0;

// Arrays to store encoder counts and velocities
const int MAX_ROWS = 10000;
long countsLeft[MAX_ROWS];
long countsRight[MAX_ROWS];
int velocitiesLeft[MAX_ROWS];
int velocitiesRight[MAX_ROWS];

// Current index for arrays
int currentIndex = 0;
int log_number = 0;


// -------------------- Setup Function --------------------

void setup() {
    // Initialize pin 50 as an input with an internal pull-up resistor
    pinMode(50, INPUT_PULLUP);  Serial.begin(115200);  // Initialize Serial Communication for debugging
    pinMode(PA_15, OUTPUT); //enable the USB-A port
    digitalWrite(PA_15, HIGH);
       
      Serial.println("Initializing Encoders...");
    
      // Initialize both encoders
      encoderLeft.begin();
      encoderRight.begin();
    
      // Read initial counts
      prevCountLeft = encoderLeft.getEncoderValue();
      prevCountRight = encoderRight.getEncoderValue();
    
      // Record the initial time
      prevTime = millis();
    
      Serial.println("Setup Complete. Starting Data Logging...");
}

// -------------------- Loop Function --------------------

void loop() {
    // Print the state to the Serial Monitor for debugging
    Serial.print("Pin 50 state: ");
    Serial.println(pinState);
    
    if (digitalRead(50) == LOW){// check if switch is pressed, then connect to usb and start logging data
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
         FILE *f = fopen("/usb/front_wheel_data.txt", "a");
         if (f == NULL) {
           Serial.print("Failed to open file: ");
           Serial.println(strerror(errno));
           while (1); // Halt execution
         }

            while (digitalRead(50) == LOW) {//Once the switch is flipped off close the text file
                unsigned long currentTime = millis();

                // Check if the specified interval has passed
                if (currentTime - prevTime >= INTERVAL) {
                    // Read current encoder counts
                    long currentCountLeft = encoderLeft.getEncoderValue();
                    long currentCountRight = encoderRight.getEncoderValue();
            
                    // Calculate the difference in counts since the last reading
                    long deltaLeft = currentCountLeft - prevCountLeft;
                    long deltaRight = currentCountRight - prevCountRight;
            
                    // Handle potential overflow for delta counts
                    const long MAX_DELTA = 10000; // Define a threshold for delta counts
                    if (abs(deltaLeft) > MAX_DELTA || abs(deltaRight) > MAX_DELTA) {
                        Serial.println("Encoder count overflow detected! Resetting counts.");
                        encoderLeft.reset();
                        encoderRight.reset();
                        prevCountLeft = encoderLeft.getEncoderValue();
                        prevCountRight = encoderRight.getEncoderValue();
                        prevTime = currentTime;
                        return; // Skip this iteration
                    }
            
                    // Calculate the time elapsed in seconds
                    float deltaTime = (currentTime - prevTime) / 1000.0;
            
                    // Calculate velocities in counts per second
                    leftVelocity = deltaLeft / deltaTime;
                    rightVelocity = deltaRight / deltaTime;
            
                    // Update previous counts and time for the next iteration
                    prevCountLeft = currentCountLeft;
                    prevCountRight = currentCountRight;
                    prevTime = currentTime;
            
                    // Convert velocities to integers by multiplying by 1000
                    leftVelocityInt = (int)(leftVelocity * 1000);
                    rightVelocityInt = (int)(rightVelocity * 1000);
            
                    // Store encoder counts as integers
                    // Note: Ensure that encoder counts fit within the int range
                    // If counts exceed, consider storing as 'long' or handle accordingly
                    countsLeft[currentIndex] = (long)currentCountLeft;
                    countsRight[currentIndex] = (long)currentCountRight;
            
                    // Store velocities
                    velocitiesLeft[currentIndex] = leftVelocityInt;
                    velocitiesRight[currentIndex] = rightVelocityInt;
            
                    // Increment the current index
                    currentIndex++;
                    // Check if arrays are full
                    if (currentIndex >= MAX_ROWS) {
                        // ADD CODE to save file to USB
                        write_data();
                        // Reset the current index for the next batch
                        currentIndex = 1;
                        log_number += 1;
                    }
                    // Output the velocities and counts to the Serial Monitor for debugging
                    Serial.print("Row: ");
                    Serial.print(currentIndex);
                    Serial.print("\tLeft Count: ");
                    Serial.print(countsLeft[currentIndex - 1]);
                    Serial.print("\tRight Count: ");
                    Serial.print(countsRight[currentIndex - 1]);
                    Serial.print("\tLeft Velocity: ");
                    Serial.print(leftVelocity);
                    Serial.print(" counts/s\tRight Velocity: ");
                    Serial.print(rightVelocity);
                    Serial.println(" counts/s");
                }
        }
        close_file()
        //If the switch is flipped back on then repeat all data logging
    }

}

void write_data(){
  err = fprintf(f, "%d,%ld,%ld,%d,%d\n", MAX_ROWS * log_number + currentIndex, countsLeft[currentIndex], countsRight[currentIndex], 
      velocitiesLeft[currentIndex], velocitiesRight[currentIndex]);
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

// -------------------- Function to Push Data to USB Drive --------------------


