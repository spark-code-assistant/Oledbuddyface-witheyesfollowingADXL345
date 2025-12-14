#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ADXL345_U.h>

// OLED Display Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ADXL345 Accelerometer
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// I2C Pins
#define SDA_PIN 21
#define SCL_PIN 22

// Calibration offsets
float xOffset = 0, yOffset = 0, zOffset = 9.8;

// Eye animation variables
float rollAngle = 0;
float pitchAngle = 0;
float zAccel = 9.8;

// Animation state
unsigned long lastBlinkTime = 0;
unsigned long blinkDuration = 150;
bool isBlinking = false;
int blinkInterval = 3000;
unsigned long animationTime = 0;

void setup() {
    Serial.begin(115200);
    delay(500);
    
    Wire.begin(SDA_PIN, SCL_PIN);
    delay(100);
    
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED init failed!");
        while (1);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Initializing...");
    display.display();
    delay(500);
    
    if (!accel.begin()) {
        Serial.println("ADXL345 not found!");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("ADXL345 Error!");
        display.display();
        while (1);
    }
    
    accel.setRange(ADXL345_RANGE_16_G);
    
    Serial.println("Calibrating... Keep device FLAT!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Calibrating...");
    display.println("Keep FLAT!");
    display.display();
    
    delay(2000);
    
    float sumX = 0, sumY = 0, sumZ = 0;
    for (int i = 0; i < 100; i++) {
        sensors_event_t event;
        accel.getEvent(&event);
        sumX += event.acceleration.x;
        sumY += event.acceleration.y;
        sumZ += event.acceleration.z;
        delay(10);
    }
    
    xOffset = sumX / 100;
    yOffset = sumY / 100;
    zOffset = sumZ / 100;
    
    Serial.println("Calibration done!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Ready!");
    display.display();
    delay(1000);
}

void loop() {
    animationTime = millis();
    
    sensors_event_t event;
    accel.getEvent(&event);
    
    float rawX = event.acceleration.x - xOffset;
    float rawY = event.acceleration.y - yOffset;
    float rawZ = event.acceleration.z;
    
    rollAngle = atan2(rawY, rawZ) * 180.0 / PI;
    pitchAngle = atan2(-rawX, sqrt(rawY*rawY + rawZ*rawZ)) * 180.0 / PI;
    zAccel = rawZ;
    
    rollAngle = constrain(rollAngle, -80, 80);
    pitchAngle = constrain(pitchAngle, -80, 80);
    
    if (millis() - lastBlinkTime > blinkInterval) {
        isBlinking = true;
        lastBlinkTime = millis();
    }
    
    if (isBlinking && millis() - lastBlinkTime > blinkDuration) {
        isBlinking = false;
        blinkInterval = random(2500, 4500);
    }
    
    display.clearDisplay();
    drawAnimatedFace(isBlinking);
    
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("R:");
    display.print((int)rollAngle);
    display.print("° P:");
    display.print((int)pitchAngle);
    display.print("°");
    
    display.display();
    delay(40);
}

void drawAnimatedFace(bool blink) {
    int centerX = 64;
    int centerY = 32;
    int eyeRadius = 11;
    int pupilRadius = 5;
    int eyeSpacing = 24;
    
    drawEyebrows(centerX - eyeSpacing, centerY - 15, centerX + eyeSpacing, centerY - 15, pitchAngle);
    
    if (blink) {
        drawClosedEye(centerX - eyeSpacing, centerY);
        drawClosedEye(centerX + eyeSpacing, centerY);
    } else {
        drawAnimatedEye(centerX - eyeSpacing, centerY, eyeRadius, pupilRadius, rollAngle, pitchAngle);
        drawAnimatedEye(centerX + eyeSpacing, centerY, eyeRadius, pupilRadius, rollAngle, pitchAngle);
    }
    
    display.drawLine(centerX, centerY - 2, centerX, centerY + 4, SSD1306_WHITE);
    
    drawAnimatedMouth(centerX, centerY + 16, pitchAngle, rollAngle);
}

void drawEyebrows(int leftX, int leftY, int rightX, int rightY, float pitchAngle) {
    float brow_tilt = (pitchAngle / 80.0) * 8;
    float brow_raise = abs(pitchAngle / 80.0) * 2;
    
    display.drawLine(leftX - 6, leftY + brow_raise, leftX + 6, leftY + brow_raise - brow_tilt, SSD1306_WHITE);
    display.drawLine(leftX - 6, leftY + 1 + brow_raise, leftX + 6, leftY + 1 + brow_raise - brow_tilt, SSD1306_WHITE);
    
    display.drawLine(rightX - 6, leftY + brow_raise + brow_tilt, rightX + 6, leftY + brow_raise, SSD1306_WHITE);
    display.drawLine(rightX - 6, leftY + 1 + brow_raise + brow_tilt, rightX + 6, leftY + 1 + brow_raise, SSD1306_WHITE);
}

void drawAnimatedEye(int x, int y, int eyeRad, int pupilRad, float rollAngle, float pitchAngle) {
    display.drawCircle(x, y, eyeRad, SSD1306_WHITE);
    
    int maxMovement = eyeRad - pupilRad - 1;
    float pupilOffsetX = (rollAngle / 80.0) * maxMovement;
    float pupilOffsetY = (pitchAngle / 80.0) * maxMovement;
    
    int pupilX = x + pupilOffsetX;
    int pupilY = y + pupilOffsetY;
    display.fillCircle(pupilX, pupilY, pupilRad, SSD1306_WHITE);
}

void drawClosedEye(int x, int y) {
    display.drawLine(x - 8, y, x + 8, y, SSD1306_WHITE);
    display.drawLine(x - 8, y + 1, x + 8, y + 1, SSD1306_WHITE);
}

void drawAnimatedMouth(int x, int y, float pitchAngle, float rollAngle) {
    float mouthOpen = abs(pitchAngle / 80.0) * 4;
    float mouthTilt = (rollAngle / 80.0) * 3;
    
    int mouthWidth = 10;
    
    for (int i = -mouthWidth; i <= mouthWidth; i++) {
        int curveY = (i * i) / (mouthWidth * 2);
        int pointY = y + curveY + mouthTilt;
        
        display.drawPixel(x + i, pointY, SSD1306_WHITE);
    }
    
    if (pitchAngle > 30) {
        display.drawLine(x - 8, y + 3, x - 3, y + 6, SSD1306_WHITE);
        display.drawLine(x + 8, y + 3, x + 3, y + 6, SSD1306_WHITE);
    } else if (pitchAngle < -30) {
        display.drawLine(x - 8, y + 6, x - 3, y + 3, SSD1306_WHITE);
        display.drawLine(x + 8, y + 6, x + 3, y + 3, SSD1306_WHITE);
    }
}
