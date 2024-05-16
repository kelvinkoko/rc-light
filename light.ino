#include "FspTimer.h"
#define IS_ENABLE(var, flag) var & flag

const int SIGNAL_LEFT_PIN = 5;
const int SIGNAL_RIGHT_PIN = 6;
const int FRONT_PIN = 7;
const int REAR_PIN = 4;
const int CH4_PIN = 2;

const int FRONT = 0x0001;
const int REAR = 0x0002;
const int LEFT = 0x0004;
const int RIGHT = 0x008;

const int CH4_FULL_LEFT = 81632;
const int CH4_LEFT = 195152;
const int CH4_MIDDLE = 264836;
const int CH4_RIGHT= 370385;
const int CH4_FULL_RIGHT= 497852;

int brightness = 0;  // how bright the LED is
int fadeAmount = 255;  // how many points to fade the LED
unsigned int ch4Value = 0;

enum Mode {
  	OFF = 0,
    FRONT_REAR_ON = FRONT | REAR,
    FRONT_REAR_LEFT_ON = FRONT_REAR_ON | LEFT,
    FRONT_REAR_RIGHT_ON = FRONT_REAR_ON | RIGHT,
    FRONT_REAR_RIGHT_EMERGENCY_ON = FRONT_REAR_ON | LEFT | RIGHT,
};

Mode currentMode = FRONT_REAR_ON;

FspTimer audio_timer;
uint64_t count=0;
uint64_t start_time=0;

// callback method used by timer
void timer_callback(timer_callback_args_t __attribute((unused)) *p_args) {
  Serial.println("timer");
}

bool beginTimer(float rate) {
  uint8_t timer_type = GPT_TIMER;
  int8_t tindex = FspTimer::get_available_timer(timer_type);
  if (tindex < 0){
    tindex = FspTimer::get_available_timer(timer_type, true);
  }
  if (tindex < 0){
    return false;
  }

  FspTimer::force_use_of_pwm_reserved_timer();

  if(!audio_timer.begin(TIMER_MODE_PERIODIC, timer_type, tindex, rate, 0.0f, timer_callback)){
    return false;
  }

  if (!audio_timer.setup_overflow_irq()){
    return false;
  }

  if (!audio_timer.open()){
    return false;
  }

  if (!audio_timer.start()){
    return false;
  }
  return true;
}

void setup() {
  pinMode(SIGNAL_LEFT_PIN, OUTPUT);
  pinMode(SIGNAL_RIGHT_PIN, OUTPUT);
  pinMode(FRONT_PIN, OUTPUT);
  pinMode(REAR_PIN, OUTPUT);
  pinMode(CH4_PIN, INPUT);

  beginTimer(2);

  Serial.begin(9600);
}

// the loop routine runs over and over again forever:
void loop() {
  int duration = pulseIn(CH4_PIN, HIGH, 1000*1000*2);
  Serial.println(duration);
  ch4Value = duration;
  if (ch4Value < CH4_FULL_LEFT) {
    currentMode = OFF;
  } else if (ch4Value < CH4_LEFT) {
    currentMode = FRONT_REAR_LEFT_ON;
  } else if (ch4Value < CH4_MIDDLE) {
    currentMode = FRONT_REAR_ON;
  } else if (ch4Value < CH4_RIGHT) {
    currentMode = FRONT_REAR_RIGHT_ON;
  } else if (ch4Value < CH4_FULL_RIGHT) {
    currentMode = FRONT_REAR_RIGHT_EMERGENCY_ON;
  }

  digitalWrite(REAR_PIN, IS_ENABLE(currentMode, REAR));
  digitalWrite(FRONT_PIN, IS_ENABLE(currentMode, FRONT));

  if (IS_ENABLE(currentMode, LEFT)) {
    analogWrite(SIGNAL_LEFT_PIN, brightness);
  } else {
    analogWrite(SIGNAL_LEFT_PIN, 0);
  }

  if (IS_ENABLE(currentMode, RIGHT)) {
    analogWrite(SIGNAL_RIGHT_PIN, brightness);
  } else {
    analogWrite(SIGNAL_RIGHT_PIN, 0);
  }

  if (IS_ENABLE(currentMode, LEFT) | IS_ENABLE(currentMode, RIGHT)) {
    // change the brightness for next time through the loop:
    brightness = brightness + fadeAmount;

    // reverse the direction of the fading at the ends of the fade:
    if (brightness <= 0 || brightness >= 255) {
      fadeAmount = -fadeAmount;
    }
    // wait for 30 milliseconds to see the dimming effect
    delay(500);
  }
}
