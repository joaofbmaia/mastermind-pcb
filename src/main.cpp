#include <Arduino.h>
#include <FastLED.h>
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>


#define NUM_LEDS_BIG 32
#define NUM_LEDS_SMALL 28
#define DATA_PIN_BIG 3 //PD3
#define DATA_PIN_SMALL 2 //PD2
#define LED_TYPE_BIG WS2812B
#define LED_TYPE_SMALL WS2812B
#define COLOR_ORDER_BIG GRB
#define COLOR_ORDER_SMALL GRB
#define BRIGHTNESS 156

//CRGB colors[8] = {CRGB::Red, CRGB::Blue, CRGB::Green, CRGB::Orange, CRGB::Magenta, CRGB::DarkViolet, CRGB::Lime, CRGB::Tomato};
CRGB colors[8] = {CHSV(0, 232, BRIGHTNESS), //red
                  CHSV(176, 232, BRIGHTNESS), //blue
                  CHSV(96, 200, BRIGHTNESS), //green
                  CHSV(32, 232, BRIGHTNESS),  //orange
                  CHSV(224, 198, BRIGHTNESS), //pink
                  CHSV(120, 208, BRIGHTNESS), //cyan
                  CHSV(80, 255, BRIGHTNESS), //lime
                  CHSV(10, 255, BRIGHTNESS)}; //tomato};

CRGB ledsBig[NUM_LEDS_BIG];
CRGB ledsSmall[NUM_LEDS_SMALL];
int guessBuffer[4] = {0, 1, 2, 3};

int *validateGuess(int *guess, int *code, int keySize);
void buttonRoutine0(void);
void buttonRoutine1(void);
void buttonRoutine2(void);
void buttonRoutine3(void);
void buttonRoutine4(void);
void changeGuessColor(int position);

int main(void) {
  init();
  void (*buttonRoutine[5]) (void) = {buttonRoutine0, buttonRoutine1, buttonRoutine2, buttonRoutine3, buttonRoutine4};
  unsigned long debounceTime = 0;
  unsigned long debounce = 50;
  int buttonReading[5];
  int buttonState[5];
  int lastButtonState[5];
  memset(lastButtonState, 0, sizeof(lastButtonState));

  DDRD &= 0b00011111; //set button pins as inputs
  DDRB &= 0b11111100;

  FastLED.addLeds<LED_TYPE_BIG, DATA_PIN_BIG, COLOR_ORDER_BIG>(ledsBig, NUM_LEDS_BIG);
  FastLED.addLeds<LED_TYPE_SMALL, DATA_PIN_SMALL, COLOR_ORDER_SMALL>(ledsSmall, NUM_LEDS_SMALL);

  for(int i = 0; i < 4; i++) ledsBig[i] = colors[guessBuffer[i]];
  for(int i = 4; i < NUM_LEDS_BIG; i++) ledsBig[i] = CRGB::Black;
  for(int i = 0; i < NUM_LEDS_SMALL; i++) ledsSmall[i] = CRGB::Black;
  FastLED.show();

  while(1) {

    buttonReading[0] = (PIND >> 5) & 1; //PD5
    buttonReading[1] = (PIND >> 6) & 1; //PD6
    buttonReading[2] = (PIND >> 7) & 1; //PD7
    buttonReading[3] = (PINB >> 0) & 1; //PB0
    buttonReading[4] = (PINB >> 1) & 1; //PB1

    //debounce buttons
    for(int i = 0; i < 5; i++) {
      if(buttonReading[i] != lastButtonState[i]) debounceTime = millis();
        if((millis() - debounceTime) > debounce && buttonReading[i] != buttonState[i]) {
          buttonState[i] = buttonReading[i];
          if(buttonState[i]) {
            (*buttonRoutine[i])();
          }
        }
    }

    memcpy(lastButtonState, buttonReading, sizeof(buttonReading));

  }
}


int *validateGuess(int *guess, int *code, int keySize) {
    static int result[2] = {0, 0}; // result[0] = black; result[1] = white;

    //positions of whites and blacks
    int blacks[keySize]; // 0 = peg in guess is same color as peg in code; 1 = peg in guess is different as peg in code
    int whites[keySize]; // 0 = theres a peg in guess with same color as this peg on code; 1 = no pegs in guess with this color
    for (int i = 0; i < keySize; i++) {
      blacks[i] = 1;
      whites[i] = 1;
    }

    //check for blacks
    for(int i = 0; i < keySize; i++) {
        if(guess[i] == code[i]) blacks[i]=0; //mark black if match found
    }

    //check for whites
    for(int i = 0; i < keySize; i++) {
        if(blacks[i]) { //skip checking if theres already a black in this position (this position in the guess already matches code)
            for(int j = 0; j < keySize; j++) { //check against every position in code only if the pegs in code are not already marked with a black or white
                if((guess[i] == code[j]) && blacks[j] && whites[j]) {
                    whites[j] = 0; //mark white if theres a peg in guess with same color as this peg on code
                    break; //breaks for loop to avoid counting repeated whites in code (if theres a color more frequent in code than guess)
                }
            }
        }
    }

    //this sections counts the whites and blacks
    for(int i = 0; i < keySize; i++) {
        if(!blacks[i]) result[0]++;
        if(!whites[i]) result[1]++;
    }

    return result;
}

void buttonRoutine0(void) {
  changeGuessColor(0);
}

void buttonRoutine1(void) {
  changeGuessColor(1);
}

void buttonRoutine2(void) {
  changeGuessColor(2);
}

void buttonRoutine3(void) {
  changeGuessColor(3);
}

void buttonRoutine4(void) {
  static int code[4];
  static int roundNumber = 0;
  int *result;
  int *guess = guessBuffer;
  if(roundNumber == 0) { //generate key
    srand(millis());
    for(int i = 0; i < 4; i++) code[i] = rand() % 6;
  }
  result = validateGuess(guess, code, 4);
  if(roundNumber < 7) { //if theres space to display all the rounds
    for(int i = 4 * (roundNumber + 1); i < 4 * (roundNumber + 2); i++) ledsBig[i] = colors[guessBuffer[i]]; //print guess
    for(int i = 4 * roundNumber; i < 4 * (roundNumber + 1); i++) { //print result
      if(result[0]) {
        ledsSmall[i] = colors[6];
        result[0]--;
      }
      else if(result[1]) {
        ledsSmall[i] = colors[7];
        result[1]--;
      }
    }
  }
  else { //theres no space left... will start to ditch older rounds
    for(int i = 4; i < NUM_LEDS_BIG - 4; i++) ledsBig[i] = ledsBig[i + 4]; //shift led array
    for(int i = 0; i < NUM_LEDS_SMALL - 4; i++) ledsSmall[i] = ledsSmall[i + 4];

    for(int i = NUM_LEDS_BIG - 4; i < NUM_LEDS_BIG; i++) ledsBig[i] = colors[guessBuffer[i]]; //print guess
    for(int i = NUM_LEDS_SMALL - 4; i < NUM_LEDS_SMALL; i++) { //print result
      if(result[0]) {
        ledsSmall[i] = colors[6];
        result[0]--;
      }
      else if(result[1]) {
        ledsSmall[i] = colors[7];
        result[1]--;
      }
    }
  }
  FastLED.show();
  roundNumber++;
  if(result[0] == 4 && result[1] == 0) { //if game is won
    for(int blink = 0; blink < 3; blink++) { //blink
      for(int i = 0; i < 4; i++) ledsBig[i] = colors[guessBuffer[i]];
      FastLED.show();
      _delay_ms(500);
      for(int i = 0; i < 4; i++) ledsBig[i] = CRGB::Black;
      FastLED.show();
      _delay_ms(500);
    }
    roundNumber = 0; //reset board
    for(int i = 0; i < 4; i++) {
      guessBuffer[i] = i;
      ledsBig[i] = colors[guessBuffer[i]];
    }
    for(int i = 4; i < NUM_LEDS_BIG; i++) ledsBig[i] = CRGB::Black;
    for(int i = 0; i < NUM_LEDS_SMALL; i++) ledsSmall[i] = CRGB::Black;
    FastLED.show();
  }
}

void changeGuessColor(int position) {
  guessBuffer[position] = (guessBuffer[position] + 1) % 6;
  for(int i = 0; i < 4; i++) ledsBig[i] = colors[guessBuffer[i]];
  FastLED.show();
}