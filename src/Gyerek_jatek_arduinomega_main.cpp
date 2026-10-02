#include "Gyerek_jatek_arduinomega_menu.h"
#include "FastLED.h" //FastLED.h Library Download
#define NUM_LEDS 70  // Total Number of LED (70+5 score leds)
#define DATA_PIN 2   // Cyclone LED Strip D2
#define SCORE_LEDS 5
#define BRIGHTNESS_HIGH 255
#define BRIGHTNESS_LOW 80
#define ANALOG_IN_PIN A0

//music notes
#define NOTE_B0 31
#define NOTE_C1 33
#define NOTE_CS1 35
#define NOTE_D1 37
#define NOTE_DS1 39
#define NOTE_E1 41
#define NOTE_F1 44
#define NOTE_FS1 46
#define NOTE_G1 49
#define NOTE_GS1 52
#define NOTE_A1 55
#define NOTE_AS1 58
#define NOTE_B1 62
#define NOTE_C2 65
#define NOTE_CS2 69
#define NOTE_D2 73
#define NOTE_DS2 78
#define NOTE_E2 82
#define NOTE_F2 87
#define NOTE_FS2 93
#define NOTE_G2 98
#define NOTE_GS2 104
#define NOTE_A2 110
#define NOTE_AS2 117
#define NOTE_B2 123
#define NOTE_C3 131
#define NOTE_CS3 139
#define NOTE_D3 147
#define NOTE_DS3 156
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_FS3 185
#define NOTE_G3 196
#define NOTE_GS3 208
#define NOTE_A3 220
#define NOTE_AS3 233
#define NOTE_B3 247
#define NOTE_C4 262
#define NOTE_CS4 277
#define NOTE_D4 294
#define NOTE_DS4 311
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_FS4 370
#define NOTE_G4 392
#define NOTE_GS4 415
#define NOTE_A4 440
#define NOTE_AS4 466
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_CS5 554
#define NOTE_D5 587
#define NOTE_DS5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_FS5 740
#define NOTE_G5 784
#define NOTE_GS5 831
#define NOTE_A5 880
#define NOTE_AS5 932
#define NOTE_B5 988
#define NOTE_C6 1047
#define NOTE_CS6 1109
#define NOTE_D6 1175
#define NOTE_DS6 1245
#define NOTE_E6 1319
#define NOTE_F6 1397
#define NOTE_FS6 1480
#define NOTE_G6 1568
#define NOTE_GS6 1661
#define NOTE_A6 1760
#define NOTE_AS6 1865
#define NOTE_B6 1976
#define NOTE_C7 2093
#define NOTE_CS7 2217
#define NOTE_D7 2349
#define NOTE_DS7 2489
#define NOTE_E7 2637
#define NOTE_F7 2794
#define NOTE_FS7 2960
#define NOTE_G7 3136
#define NOTE_GS7 3322
#define NOTE_A7 3520
#define NOTE_AS7 3729
#define NOTE_B7 3951
#define NOTE_C8 4186
#define NOTE_CS8 4435
#define NOTE_D8 4699
#define NOTE_DS8 4978
#define REST 0

// change this to make the song slower or faster
int tempo = 70;

// change this to whichever pin you want to use
int buzzer = 5;

// notes of the moledy followed by the duration.
// a 4 means a quarter note, 8 an eighteenth , 16 sixteenth, so on
// !!negative numbers are used to represent dotted notes,
// so -4 means a dotted quarter note, that is, a quarter plus an eighteenth!!
int melody[] = {

    // Greensleeves
    // https://github.com/robsoncouto/arduino-songs/blob/master/greensleeves/greensleeves.ino
    // Score available at https://musescore.com/user/168402/scores/1396946
    // Alexander Trompoukis

    NOTE_G4, 8, // 1
    NOTE_AS4, 4, NOTE_C5, 8, NOTE_D5, -8, NOTE_DS5, 16, NOTE_D5, 8,
    NOTE_C5, 4, NOTE_A4, 8, NOTE_F4, -8, NOTE_G4, 16, NOTE_A4, 8,
    NOTE_AS4, 4, NOTE_G4, 8, NOTE_G4, -8, NOTE_FS4, 16, NOTE_G4, 8,
    NOTE_A4, 4, NOTE_FS4, 8, NOTE_D4, 4, NOTE_G4, 8,

    NOTE_AS4, 4, NOTE_C5, 8, NOTE_D5, -8, NOTE_DS5, 16, NOTE_D5, 8, // 6
    NOTE_C5, 4, NOTE_A4, 8, NOTE_F4, -8, NOTE_G4, 16, NOTE_A4, 8,
    NOTE_AS4, -8, NOTE_A4, 16, NOTE_G4, 8, NOTE_FS4, -8, NOTE_E4, 16, NOTE_FS4, 8,
    NOTE_G4, -2,
    NOTE_F5, 2, NOTE_E5, 16, NOTE_D5, 8,

    NOTE_C5, 4, NOTE_A4, 8, NOTE_F4, -8, NOTE_G4, 16, NOTE_A4, 8, // 11
    NOTE_AS4, 4, NOTE_G4, 8, NOTE_G4, -8, NOTE_FS4, 16, NOTE_G4, 8,
    NOTE_A4, 4, NOTE_FS4, 8, NOTE_D4, 04,
    NOTE_F5, 2, NOTE_E5, 16, NOTE_D5, 8,
    NOTE_C5, 4, NOTE_A4, 8, NOTE_F4, -8, NOTE_G4, 16, NOTE_A4, 8,

    NOTE_AS4, -8, NOTE_A4, 16, NOTE_G4, 8, NOTE_FS4, -8, NOTE_E4, 16, NOTE_FS4, 8, // 16
    NOTE_G4, -2,

    // repeats from the beginning

    NOTE_G4, 8, // 1
    NOTE_AS4, 4, NOTE_C5, 8, NOTE_D5, -8, NOTE_DS5, 16, NOTE_D5, 8,
    NOTE_C5, 4, NOTE_A4, 8, NOTE_F4, -8, NOTE_G4, 16, NOTE_A4, 8,
    NOTE_AS4, 4, NOTE_G4, 8, NOTE_G4, -8, NOTE_FS4, 16, NOTE_G4, 8,
    NOTE_A4, 4, NOTE_FS4, 8, NOTE_D4, 4, NOTE_G4, 8,

    NOTE_AS4, 4, NOTE_C5, 8, NOTE_D5, -8, NOTE_DS5, 16, NOTE_D5, 8, // 6
    NOTE_C5, 4, NOTE_A4, 8, NOTE_F4, -8, NOTE_G4, 16, NOTE_A4, 8,
    NOTE_AS4, -8, NOTE_A4, 16, NOTE_G4, 8, NOTE_FS4, -8, NOTE_E4, 16, NOTE_FS4, 8,
    NOTE_G4, -2,
    NOTE_F5, 2, NOTE_E5, 16, NOTE_D5, 8,

    NOTE_C5, 4, NOTE_A4, 8, NOTE_F4, -8, NOTE_G4, 16, NOTE_A4, 8, // 11
    NOTE_AS4, 4, NOTE_G4, 8, NOTE_G4, -8, NOTE_FS4, 16, NOTE_G4, 8,
    NOTE_A4, 4, NOTE_FS4, 8, NOTE_D4, 04,
    NOTE_F5, 2, NOTE_E5, 16, NOTE_D5, 8,
    NOTE_C5, 4, NOTE_A4, 8, NOTE_F4, -8, NOTE_G4, 16, NOTE_A4, 8,

    NOTE_AS4, -8, NOTE_A4, 16, NOTE_G4, 8, NOTE_FS4, -8, NOTE_E4, 16, NOTE_FS4, 8, // 16
    NOTE_G4, -2

};

// sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
// there are two values per note (pitch and duration), so for each note there are four bytes
int notes = sizeof(melody) / sizeof(melody[0]) / 2;

// this calculates the duration of a whole note in ms
int wholenote = (60000 * 4) / tempo;

int divider = 0, noteDuration = 0;

//Cyclon Game variables
CRGB leds[75];  //Cyclotn + Score LEDs
byte gameState = 0;
const byte ledSpeed[4] = {50, 40, 35, 20};
bool findRandom = false;
byte spot = 0;
int period = 1000;
unsigned long time_now = 0;
unsigned long time_now1 = 0;
unsigned long time_now2 = 0;
unsigned long time_now3 = 0;
byte Position = 0;
byte level = 0;
unsigned long  tesztelo_counter=0;
taskid_t tesztelo_taskid;

//Function prototypes
void CyclonPlayGame(byte bound1, byte bound2);
void clearLEDS();
void CyclonWinner();
void CyclonLoser();
void CyclonGame();
String ioDeviceDigitalScan();


void setup()
{
  //cyclon game led setup
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS+SCORE_LEDS);
  //FastLED.addLeds<WS2812B, DATA_PIN, GRB>(sleds, SCORE_LEDS);

  AnalogDevice &analog = internalAnalogDevice();
  internalAnalogDevice().initPin(ANALOG_IN_PIN, DIR_IN);
  

  pinMode(1, INPUT_PULLUP);
  //pinMode(2, INPUT_PULLUP); PWM LED!
  pinMode(3, INPUT_PULLUP); // Push Button D4
  pinMode(4, INPUT_PULLUP); // Enable game
  //pinMode(5, INPUT_PULLUP); // Enable music
  pinMode(6, INPUT_PULLUP); 
  pinMode(7, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(11, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
  pinMode(15, INPUT_PULLUP);
  pinMode(16, INPUT_PULLUP);
  pinMode(17, INPUT_PULLUP);
  pinMode(18, INPUT_PULLUP);
  pinMode(19, INPUT_PULLUP);
  pinMode(20, INPUT_PULLUP);
  pinMode(21, INPUT_PULLUP);
  pinMode(22, INPUT_PULLUP);
  pinMode(23, INPUT_PULLUP);
  pinMode(24, INPUT_PULLUP);
  pinMode(25, INPUT_PULLUP);
  pinMode(26, INPUT_PULLUP);
  pinMode(27, INPUT_PULLUP);
  pinMode(28, INPUT_PULLUP);
  pinMode(29, INPUT_PULLUP);
  pinMode(30, INPUT_PULLUP);
  pinMode(31, INPUT_PULLUP);
  pinMode(32, INPUT_PULLUP);
  pinMode(33, INPUT_PULLUP);
  pinMode(34, INPUT_PULLUP);
  pinMode(35, INPUT_PULLUP);
  pinMode(36, INPUT_PULLUP);
  pinMode(37, INPUT_PULLUP);
  pinMode(38, INPUT_PULLUP);
  pinMode(39, INPUT_PULLUP);
  pinMode(40, INPUT_PULLUP);
  pinMode(41, INPUT_PULLUP);
  pinMode(42, INPUT_PULLUP);
  pinMode(43, INPUT_PULLUP);
  pinMode(44, INPUT_PULLUP);
  pinMode(45, INPUT_PULLUP);
  pinMode(46, INPUT_PULLUP);
  pinMode(47, INPUT_PULLUP);
  pinMode(48, INPUT_PULLUP);
  pinMode(49, INPUT_PULLUP);
  pinMode(50, INPUT_PULLUP);
  pinMode(51, INPUT_PULLUP);
  pinMode(52, INPUT_PULLUP);


  FastLED.setBrightness(BRIGHTNESS_LOW);

  Serial.begin(9600);
  Serial.println("Reset");

  // zene

  setupMenu();
  //  internalDigitalDevice().digitalReadS()
  // const int ledOutputPin = LED_BUILTIN;
  // internalAnalogDevice().initPin(10, DIR_IN);
  // internalDigitalDevice().pinMode(11, INPUT);
  // Serial.println(internalDigitalDevice().readValue(11));
  // Serial.println(internalDigitalDevice().digitalRead(11));

  // ioDeviceDigitalRead()
  taskManager.scheduleFixedRate(50, []()
                                    {
    CyclonGame();

    // Serial.println(ioDeviceDigitalScan());
    // Serial.println(internalDigitalDevice().digitalRead(11));
    // Serial.println(internalAnalogDevice().readValue(10)); 
    });
}

void loop()
{
  taskManager.runLoop();
}

void CyclonPlayGame(byte bound1, byte bound2)
{
  Serial.println("Start game");
  leds[Position].setRGB(255, 0, 0);
  if (Position < bound1 + 1 || Position > bound2 + 1)
  {
    leds[Position - 1].setRGB(0, 0, 0);
  }
  FastLED.show();
  Position++;
  if (Position >= NUM_LEDS)
  {
    leds[Position - 1].setRGB(0, 0, 0);
    Position = 0;
  }
}

void CyclonWinner()
{
  for (byte i = 0; i < 3; i++)
  {
    FastLED.setBrightness(BRIGHTNESS_HIGH);
    for (byte j = 0; j < NUM_LEDS; j++)
    {
      leds[j].setRGB(0, 255, 0);
    }
    //generate none blocking delay
    FastLED.show();
    time_now1 = millis();
    if (millis() - time_now1 >= 500)
    {
      clearLEDS();
    }
    time_now2 = millis();
    if (millis() - time_now2 >= 500){
      FastLED.show();
    }
    time_now3 = millis();
    if (millis() - time_now3 >= 500)
    {
      clearLEDS();
    }
        }
  findRandom = true;
  Position = 0;

  gameState = level + 1;
  if (gameState > 4)
  {
    gameState = 0;
  }
  FastLED.setBrightness(BRIGHTNESS_LOW);
}
void CyclonLoser()
{
  FastLED.setBrightness(BRIGHTNESS_HIGH);

  for (byte i = 0; i < 3; i++)
  {
    for (byte j = 0; j < NUM_LEDS; j++)
    {
      leds[j].setRGB(255, 0, 0);
    }
    FastLED.show();
    time_now = millis();
    if (millis() - time_now >= 500)
    {
      clearLEDS();
      FastLED.show();
    }
  }
  gameState = 0;
  FastLED.setBrightness(BRIGHTNESS_LOW);
}
void clearLEDS()
{
  for (byte i = 0; i < NUM_LEDS; i++)
  {
    leds[i].setRGB(0, 0, 0);
  }
}

void CyclonGame()
{ // put your main code here, to run repeatedly:
  {
    Serial.println("cyclon game");
    if (gameState == 0) //random LED
    {
      FastLED.setBrightness(10);
      fill_rainbow(leds, NUM_LEDS+SCORE_LEDS, 10, 15);   // 2 = longer gradient strip
      //fill_rainbow(sleds, SCORE_LEDS, 0, 7); // 2 = longer gradient strip
      if (digitalRead(46) == LOW)
      {
        Serial.println("PRESS 4");
        Position = 0;
        findRandom = true;
        time_now = millis();
        if (millis() - time_now >= 500)
        {
          for (byte i = 0; i < NUM_LEDS; i++)
          {
            leds[i].setRGB(0, 0, 0);
            delay(40);
            FastLED.show();
          }
        }
        for (byte i = 71; i < NUM_LEDS+SCORE_LEDS; i++) //SCORE LEDS
        {
          leds[i].setRGB(0, 0, 0);
          delay(100);
          FastLED.show();
        }
        gameState = 1;
      }
      FastLED.show();
    }
    if (gameState == 1) //start new game
    {
      period = ledSpeed[0];
      if (millis() > time_now + period)
      {
        time_now = millis();
        if (findRandom)
        {
          spot = random(44) + 3; // Find random LED
          findRandom = false;
        }
        FastLED.setBrightness(BRIGHTNESS_HIGH);
        leds[spot - 2].setRGB(255, 140, 0);
        leds[spot - 1].setRGB(255, 140, 0);
        leds[spot].setRGB(0, 255, 0);
        leds[spot + 1].setRGB(255, 140, 0);
        leds[spot + 2].setRGB(255, 140, 0);
        leds[70].setRGB(0, 0, 255); //SCORE LED 1. POINT
        CyclonPlayGame(spot - 1, spot + 1);
        FastLED.setBrightness(BRIGHTNESS_LOW);
      }
      if (digitalRead(46) == LOW)
      {
        Serial.println("PRESS 4");

        delay(300);
        findRandom = false;
        if (Position > spot - 2 && Position < spot + 4)
        {
          level = gameState;
          gameState = 98; //CyclonWinner
        }
        else
        {
          gameState = 99; //CyclonLoser
        }
      }
    }
    if (gameState == 2)
    {
      //    period = 320;
      period = ledSpeed[1];
      if (millis() > time_now + period)
      {
        time_now = millis();
        if (findRandom)
        {
          spot = random(44) + 3; // Find random LED
          findRandom = false;
        }
        FastLED.setBrightness(BRIGHTNESS_HIGH);
        leds[spot - 1].setRGB(255, 140, 0);
        leds[spot].setRGB(0, 255, 0);
        leds[spot + 1].setRGB(255, 140, 0);
        leds[70+1].setRGB(0, 0, 255); //SCORE LED 2. POINT
        CyclonPlayGame(spot - 1, spot + 1);
        FastLED.setBrightness(BRIGHTNESS_LOW);
      }
      if (digitalRead(46) == LOW)
      {
        Serial.println("PRESS 4");

        delay(300);
        if (spot - 1 && Position < spot + 3)
        {
          level = gameState;
          gameState = 98; //CyclonWinner
        }
        else
        {
          gameState = 99; //CyclonLoser
        }
      }
    }
    if (gameState == 3)
    {
      period = ledSpeed[2];
      if (millis() > time_now + period)
      {
        time_now = millis();
        if (findRandom)
        {
          spot = random(44) + 3; // Find random LED
          findRandom = false;
        }
        leds[spot].setRGB(0, 255, 0);
        leds[70+2].setRGB(0, 0, 255); //SCORE LED 3. POINT
        CyclonPlayGame(spot, spot);
      }
      if (digitalRead(46) == LOW)
      {
        delay(300);
        if (Position == spot + 1)
        {
          level = gameState;
          gameState = 98;
        }
        else
        {
          gameState = 99;
        }
      }
    }
    if (gameState == 4)
    {
      period = ledSpeed[3];
      if (millis() > time_now + period)
      {
        time_now = millis();
        if (findRandom)
        {
          spot = random(44) + 3; // Find random LED
          findRandom = false;
        }
        FastLED.setBrightness(BRIGHTNESS_HIGH);
        leds[spot].setRGB(0, 255, 0);
        leds[70+3].setRGB(0, 0, 255); //SCORE LED 4. POINT
        CyclonPlayGame(spot, spot);
        FastLED.setBrightness(BRIGHTNESS_LOW);
      }
      if (digitalRead(46) == LOW)
      {
        delay(300);
        if (Position == spot + 1)
        {
          level = gameState;
          gameState = 98;
        }
        else
        {
          gameState = 99;
        }
      }
    }
    if (gameState == 98)
    {
      CyclonWinner();
    }
    if (gameState == 99)
    {
      CyclonLoser();
    }
  }
}

void CALLBACK_FUNCTION Gomb1(int id)
{
  if (menuGomb1.getCurrentValue() == true)
    Serial.println("gomb1 true");
  Serial.println("pin46-52");
  Serial.println(digitalRead(46));
  Serial.println( digitalRead(47));
  Serial.println( digitalRead(48));
  Serial.println( digitalRead(49));
  Serial.println( digitalRead(50));
  Serial.println( digitalRead(51));
  Serial.println( digitalRead(52));
  menuDigIn.setTextValue("p1_textval");
  //menuNewActionItem.
  //menuNewRuntimeList.
}


// This callback needs to be implemented by you, see the below docs:
//  1. List Docs - https://tcmenu.github.io/documentation/arduino-libraries/tc-menu/menu-item-types/list-menu-item/
//  2. ScrollChoice Docs - https://tcmenu.github.io/documentation/arduino-libraries/tc-menu/menu-item-types/scrollchoice-menu-item/
int CALLBACK_FUNCTION fnNewRuntimeListRtCall(RuntimeMenuItem* item, uint8_t row, RenderFnMode mode, char* buffer, int bufferSize) {
    switch(mode) {
    default:
        return defaultRtListCallback(item, row, mode, buffer, bufferSize);
    }
}


void CALLBACK_FUNCTION DigIn_(int id) {

  
}

void CALLBACK_FUNCTION Tesztelo(int id) {
  //if( tesztelo_counter>0) taskManager.cancelTask(tesztelo_taskid);
 // Serial.println(tesztelo_taskid);
  //tesztelo_counter = 199;
  //Serial.println("cancel task2");
}


void CALLBACK_FUNCTION DigIn(int id) {
 menuDigIn.setTextValue(ioDeviceDigitalScan().c_str());
}

String ioDeviceDigitalScan()
{
  String result = "";
  for (size_t i = 46; i < 52; i++)
  {
    //read pull down pins
    if (!internalDigitalDevice().digitalRead(i))
    {
      result += String(i) + " ";
      if(result.length()>3) result="DUPLA!";
    }
  }
  return result;
}


void CALLBACK_FUNCTION AnIn(int id) {
  menuAnIn.setTextValue(String(internalAnalogDevice().getCurrentValue(ANALOG_IN_PIN)).c_str());
                                  serdebugF4("Analog input value is ", internalAnalogDevice().getCurrentValue(ANALOG_IN_PIN),
                                             internalAnalogDevice().getMaximumRange(DIR_IN, ANALOG_IN_PIN),
                                             internalAnalogDevice().getCurrentFloat(ANALOG_IN_PIN) * 100.0F);

  #ifdef ESP32
                                  // On ESP32 boards, where the analogWrite function doesn't exist we use the underlying functions
                                  // to access either the DAC or LEDC subsystem, if you want to get hold of the ledc channel you can.
                                  EspAnalogOutputMode *outputMode = internalAnalogDevice().getEspOutputMode(PWM_OR_DAC_PIN);
                                  if (outputMode != nullptr)
                                  {
                                    serdebugF2("ESP32 Output type: ", outputMode->isDac());
                                    serdebugF2("LEDC (pwm channel): ", outputMode->getPwmChannel());
                                  }

                                  EspAnalogInputMode *inputMode = internalAnalogDevice().getEspInputMode(ANALOG_IN_PIN);
                                  if (inputMode != nullptr)
                                  {
                                    serdebugF3("ESP32 Input (ondac1, channel): ", inputMode->isOnDAC1(), inputMode->getChannel());
                                  }
#endif
                                
}


void CALLBACK_FUNCTION Jatek(int id) {
  Serial.println(menuJatek.getCurrentValue());
  // getCurrentValue() returns a numeric index (uint16_t), so convert to String before comparing to text
  if (menuJatek.getCurrentValue() == 0) //Cyclon
  {
   //  CyclonGame();

    taskManager.scheduleFixedRate(50, []
                                  { CyclonGame(); });
  }
  //Memory
  {
    static int thisNote = 0;
    static unsigned long noteStartTime = 0;
    static bool notePlaying = false;

    taskManager.scheduleFixedRate(3, []
                                  { 
    if (thisNote < notes * 2)
    {
      if (!notePlaying)
      {
        // Calculate note duration
        divider = melody[thisNote + 1];
        if (divider > 0)
        {
          noteDuration = (wholenote) / divider;
        }
        else if (divider < 0)
        {
          noteDuration = (wholenote) / abs(divider);
          noteDuration *= 1.5;
        }
        
        // Play the note
        tone(buzzer, melody[thisNote], noteDuration * 0.9);
        noteStartTime = millis();
        notePlaying = true;
      }
      
      // Check if note duration has elapsed
      if (millis() - noteStartTime >= noteDuration)
      {
        noTone(buzzer);
        thisNote += 2;
        notePlaying = false;
      }
    }
    else
    {
      // Song finished, reset for next play
      thisNote = 0;
      notePlaying = false;
      gameState = 0;
    }
  }
);
  }
}


void CALLBACK_FUNCTION DigOut(int id) {
    // TODO - your menu change code
}
