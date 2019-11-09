// TODO: Timer with buttons
// TODO: Morse tests with stage controll

#include <LiquidCrystal.h>
#include "OutputStorage.h"

// pins
const int strike1Pin = 6;
const int strike2Pin = 7;
const int strike3Pin = 8;
const int wrongPin = 9;
const int solvedPin = 10;
const int clickPin = 13;

// sys
int strikes = 0;
boolean exploded = false;
boolean passed = false;
const long bombTime = 1000L * 60 * .15; // mins

boolean pressed = false;
uint16_t pressedTime = 0;

boolean holding = false;
String holdingColor = "";

uint16_t strikeBlinkingTime = 100;
uint16_t strikeBlinkingTimeL = 0;
long strikeBlinkingIterations = 0;

uint16_t passBombTime = 0;

//
OutputStorage outputStorage = OutputStorage();

String buttonText[] = {
  "Hold",
  "Abort",
  "Detonate"
};

String buttonColor[] = {
  "Blue",
  "White",
  "Yellow",
  "Red"
};

String holdColors[] = {
  "Blue",
  "White",
  "Yellow",
  "Black",
  "Red"
};

String buttonConfig[2] = {};

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
  randomSeed(analogRead(0));
  Serial.begin(9600); // transport freq
  lcd.begin(16, 2);

  // pins
  pinMode(strike1Pin, OUTPUT);
  pinMode(strike2Pin, OUTPUT);
  pinMode(strike3Pin, OUTPUT);
  pinMode(wrongPin, OUTPUT);
  pinMode(solvedPin, OUTPUT);
  pinMode(clickPin, INPUT);

  // button conf
  // text
  buttonConfig[0] = buttonText[ random(sizeof(buttonText) / sizeof(String)) ];
  // color
  buttonConfig[1] = buttonColor[ random(sizeof(buttonColor) / sizeof(String)) ];
}

String nollFix(String v) {
  if (v.length() == 1) return "0" + v;
  return v;
}

uint16_t secondsLeft() {
  return (bombTime - millis()) / 1000;
}

void onClick() {
  if(exploded) return;
  
  String col = buttonConfig[1];
  String text = buttonConfig[0];

  /*
    . If the button is blue and the button says "Abort", hold the button and refer to "Releasing a Held Button".
    . If the button is yellow, hold the button and refer to "Releasing a Held Button".
    . If the button is red and the button says "Hold", press and immediately release the button.
    . If none of the above apply, hold the button and refer to "Releasing a Held Button".

  */

  if (col == "Blue" && text == "Abort") requireHolding();
  else if (col == "Yellow") requireHolding();
  else if (col == "Red" && text == "Hold"); // wait for onRelease();
  else requireHolding();
}

void requireHolding() {
  if (holding) return;

  holding = true;
  holdingColor = holdColors[ random(0, sizeof(holdColors) / sizeof(String)) ];
}

void onRelease() {
  if(exploded) return;

  int releaseGain = 600; // ms

  if (holding) { // should be holded? // XXX
    /*
      "Blue" - 4,
      "White" - 1,
      "Yellow" - 5,
      Other - 1
    */
    String waitingFor = "1";
    if (holdingColor == "Blue") waitingFor = "4";
    else if (holdingColor == "White") waitingFor = "1";
    else if (holdingColor == "Yellow") waitingFor = "5";

    if (getTimeOutput().indexOf(waitingFor) != -1) {
      passBomb();
    } else {
      countStrike();
    }
  } else if (millis() - pressedTime <= 600) { // fast release
    passBomb();
  } else { // BOOM! BOOM!
    countStrike();
  }
}

void passBomb() {
  passed = true;
  strikes = 0;
  passBombTime = secondsLeft();
}

void explodeBomb() {  
  exploded = true;
  startStrikeBlinking(2 ^ 3185);
}

void countStrike() {
  if (++strikes >= 3) { // out of strikes
    explodeBomb();
  } else {
    startStrikeBlinking(4);
  }
}

void startStrikeBlinking(int v) {
  strikeBlinkingIterations = v * 2;
}

String getTimeOutput() {
  if(exploded) return "XX:XX";
  
  const uint16_t timeLeft = (!passed) ? secondsLeft() : passBombTime;
  const String timeLeftM = nollFix(String( (int) floor(timeLeft / 60) ));
  const String timeLeftS = nollFix(String( (int) floor(timeLeft % 60) ));
  const String timeOutput = timeLeftM + ":" + timeLeftS;

  return timeOutput;
}

void loop() {
  // time & ifno
  String timeOutput = getTimeOutput();
  if (passed) timeOutput += "|PASSED";
  else if (exploded) timeOutput += "|EXPLODED";
  else if (strikes > 0) timeOutput += "|" + String(strikes) + " strike" + ((strikes > 1) ? "s" : "");
  // button
  const String buttonOutput = (!exploded && !passed) ? (!holding) ? buttonConfig[1] + ": " + buttonConfig[0] : "|" + holdingColor : "";
  // check if display should be rerendered
  const boolean rerender = timeOutput != outputStorage.getTime() || buttonOutput != outputStorage.getButton();

  if (rerender) {
    lcd.clear();

    // time
    lcd.setCursor(0, 0); // 1 symbol, 1 line
    lcd.print(timeOutput);
    outputStorage.setTime(timeOutput);

    // button info
    lcd.setCursor(0, 1); // 1 symbol, 2 line
    lcd.print(buttonOutput);
    outputStorage.setButton(buttonOutput);
  }

  // strikes & passed
  if (strikes >= 1 && !exploded) digitalWrite(strike1Pin, HIGH);
  if (strikes >= 2 && !exploded) digitalWrite(strike2Pin, HIGH);
  if (strikes == 3 && !exploded) digitalWrite(strike3Pin, HIGH);

  // strike blinking
  if (strikeBlinkingIterations && millis() - strikeBlinkingTimeL >= strikeBlinkingTime) {
    int nextState = (digitalRead(wrongPin) != HIGH) ? HIGH : LOW;
    // perform the action
    digitalWrite(wrongPin, nextState);
    if (exploded) {
      digitalWrite(strike1Pin, nextState);
      digitalWrite(strike2Pin, nextState);
      digitalWrite(strike3Pin, nextState);
    }

    // execute a next iteration
    if (--strikeBlinkingIterations > 0) {
      strikeBlinkingTimeL = millis();
    }
  }

  if (passed && !exploded) digitalWrite(solvedPin, HIGH);

  // button click
  boolean _pressed = digitalRead(clickPin) == 1;
  if (!pressed && _pressed) {
    onClick();
    pressed = _pressed;
    pressedTime = millis();
  } else if (pressed && !_pressed) {
    onRelease();
    pressed = _pressed;
  }

  if (!passed && !exploded && secondsLeft() <= 0) explodeBomb();
}
