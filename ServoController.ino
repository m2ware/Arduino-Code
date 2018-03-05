/* Servo automation

 created 2014
 by Jeff Moore
 */
 
//#include  <LiquidCrystal.h>

  // constants won't change. Used here to set pin numbers:
  // Pin 13: Arduino has an LED connected on pin 13
  // Pin 13: Teensy 3.0 has the LED on pin 13
const int ledPin =  13;      // the number of the LED pin
const int lookEnablePin = 19;
const int pwmPin = 20;
const int jawPin = 21;
const int motionPin = 11;
const int motionModulus = 100;
int motionTimer=0;
//const int motionDetectedPin = 10;

const int ledRedPin = 3;
const int ledGreenPin = 4;
const int ledBluePin = 5;

const byte soundBankPin[4] = {7,8,9,10};
#define SOUNDBANK0 3
#define SOUNDBANK1 3
#define SOUNDBANK2 3
#define SOUNDBANK3 3
const int numSoundPins = 4;

bool lookEnabled = true;
bool jawOpen = true;
bool chatterOn = false;

enum LookMode { 
  Pause =      0,
  SlowLook =   1,
  Neutral =    2,
  Shake =      3,
  RandomLook = 4,
  FastShake =  5,
  LeftRight =  6
} lookMode, previousMode;

typedef enum LcdPinEnum {
  Pin8_RS =     9,
  Pin9_Enable = 10,
  Pin4_DB4 =    16,
  Pin5_DB5 =    15,
  Pin6_DB6 =    19, 
  Pin7_DB7 =    22
} LcdPinEnum;
//LiquidCrystal lcd(Pin8_RS, Pin9_Enable, Pin4_DB4, Pin5_DB5, Pin6_DB6, Pin7_DB7);

const byte colors[9][3] = {
  { 0,   0,   0   },         // black
  { 180, 255, 255 },         // white
  { 255, 0,   0   },         // red
  { 0,   255, 0   },         // green
  { 0,   0,   255 },         // blue
  { 140, 255, 0   },         // yellow
  { 140, 0,   255 },         // purple
  { 0,   255, 255 },         // cyan
  { 200, 240, 0   }          // orange
};        
  
enum ColorMap {
  Black = 0,
  White = 1,
  Red = 2,
  Green = 3,
  Blue = 4,
  Yellow = 5,
  Purple = 6,
  Cyan = 7,
  Orange = 8 
} colorMap;

const int nColors = 9;

byte color[3] = {32, 64, 0};
byte targetColor[3] = {32, 64, 0};

// Frequency of PWM signal
const int f0 = 196;
const double T0Us = 1000000 / (255*f0);

// Min/max guardrails for PWM 
// Converted to pulse width (normalized) 0-255
const int tMinUs = 900;  // min pulse width uS
const int tMaxUs = 2100; // max pulse width uS
const int tNeutral = 1500;

const int posMax = tMaxUs / T0Us;
const int posMin = tMinUs / T0Us;
//const int posNeutral = (255*f0)/666;
//const int posNeutral = (posMax+posMin)/2;
const int posNeutral = tNeutral / T0Us;

const int jawNeutralPos = tNeutral / T0Us;
const int jawOpenPos = 1100 / T0Us;
const int jawClosedPos = 1600 / T0Us;

int chatterWaitMs = 50;

const int delta = 300;

int timerModulus = 50;
int ledModulus = 5;
int motionCooldown = 0;
int motionDetectCounter = 0;
const int cooldownTimeMs = 750;

int agitation = 0;
int cyclesOnState = 0;
int cyclesOnPreviousState = 0;
int minPauseCycles = 5;

typedef enum KeypadButtonEnum {
  BtnNone =  0,
  BtnUp   =  1,
  BtnDown =  2,
  BtnLeft =  3,
  BtnRight = 4,
  BtnSelect= 5
} KeypadButton;

KeypadButton keypadButton;

// Counter 
int pwmCounter = 0;
int duration = 2;
int dutyCycle = 0;

int pauseCounter = 0;

int position = (posMin+posMax)/2;
int newPos = position;

int slowCounter = 0;

boolean goLeft = 0;

boolean modeUp = 0;
int ledState = LOW;             // ledState used to set the LED
long previousMillis = 0;        // will store last time LED was updated

// the follow variables is a long because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long interval = 75;           // interval at which to blink (milliseconds)

void setColorTo(byte *toRgb, const byte *fromRgb, const byte divider = 1);
void setColorRgb(const byte *rgb, const byte divider = 1);

void setup() {

  //delay(2000);
  pinMode(ledPin, OUTPUT);
  pinMode(pwmPin, OUTPUT);
  pinMode(jawPin, OUTPUT);
  //pinMode(extLedPin, OUTPUT);
  pinMode(motionPin, INPUT);
  pinMode(lookEnablePin, INPUT);
  //pinMode(motionDetectedPin, OUTPUT);
  pinMode(ledRedPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);
  pinMode(ledBluePin, OUTPUT);
  
  analogWriteFrequency(pwmPin, f0);
  analogWriteFrequency(jawPin, f0);
  //digitalWrite(extLedPin, HIGH);

  initSoundPins();
  
  Serial.print("Min: ");
  Serial.println(posMin);
  Serial.print("Max: ");
  Serial.println(posMax);
  Serial.println("XXXX");

  lookMode = SlowLook;
  previousMode = SlowLook;
  changeModeTo( SlowLook );
}

void initSoundPins()
{
  for (int i = 0; i < numSoundPins; i++)
  {
    pinMode(soundBankPin[i], OUTPUT);
    digitalWrite(soundBankPin[i],HIGH);
  }
}

void triggerSound(int soundBank)
{
  Serial.print("Triggering bank #");
  Serial.print(soundBank);
  Serial.print(" on pin ");
  Serial.println(soundBankPin[soundBank]);
  digitalWrite(soundBankPin[soundBank],LOW);
  delay(200);
  digitalWrite(soundBankPin[soundBank],HIGH);
}

void ledBlink()
{
  if (ledState == LOW)
  {
    ledState = HIGH;
  }
  else 
  {
    ledState = LOW;
  }
}

void pauseLook(int nCycles)
{
   minPauseCycles = nCycles;
   changeModeTo(Pause);
}

void randomizeLookTarget()
{
    newPos = posMin + random(1+posMax-posMin);
    Serial.print("[TGT:");
    Serial.print(newPos);
    Serial.print("]");
}

void steer()
{
  if ( position == newPos )
  {
    pauseLook(5+random(5));
    //if (previousMode == RandomLook && random(4) < 1) chatterOn = true;
  }
}

void steerLeftRight(int slowCounterDelay)
{
  if (slowCounter++ >= slowCounterDelay)
  {
    //if ( random(10) < 1) pauseLook(random(20));
    if (goLeft) 
    {
      //newPos = posMin;
      newPos = posNeutral + delta;
      goLeft = false;
    } else {
      //newPos = posMax;
      newPos = posNeutral - delta;
      goLeft = true;
    }
    slowCounter = 0;
  } 
}

void shake()
{
   if (slowCounter++ >= 1)
   { 
      if (goLeft)
      {
        newPos = posNeutral-delta;
        slowCounter = 0;
        goLeft = false;
      } else {
        newPos = posNeutral+delta;
        goLeft = true;
      }
      slowCounter = 0;
   }
}

void steerNeutral()
{
  newPos = posNeutral;
}

void changeModeTo(int newModeInt)
{
  cyclesOnPreviousState = cyclesOnState;
  cyclesOnState = 0;
  jawStill();
  
  LookMode newMode = (LookMode) newModeInt;
  previousMode = lookMode;
  lookMode = newMode;
  
  Serial.flush();
  Serial.println("");
  Serial.print("[PrMode:");
  Serial.print(previousMode);
  Serial.print("]");
  Serial.print("[LkMode:");
  Serial.print(lookMode);
  Serial.println("]");
  Serial.flush();
  
  //switch (lookMode)
  switch (newModeInt)
  {
    case Pause:
       timerModulus = 40;
       setColorTo( targetColor, targetColor, 3);
       break;
    case SlowLook:
       timerModulus = 40;
       agitation=0;
       setColorTo( targetColor, colors[ random(nColors-2)+1 ], 16 );
       randomizeLookTarget();
       break;
    case RandomLook:
       timerModulus = 15;
       if (random(3) < 1) triggerSound(SOUNDBANK2);
       setColorTo( targetColor, colors[ random(nColors-2)+1 ], 3 );
       randomizeLookTarget();
       break;
    case LeftRight:
       timerModulus = 20;
       triggerSound(SOUNDBANK1);
       setColorTo( targetColor, colors[ Orange ], 2 );
       break;
    case FastShake:
       timerModulus = 10;
       triggerSound(SOUNDBANK3);
       setColorTo( targetColor, colors[ Cyan ], 1 );
       chatterOn = true;
       break;
    case Shake:
       timerModulus = 25;
       triggerSound(SOUNDBANK1);
       setColorTo( targetColor, colors[ random(nColors-2)+1 ], 3 );
       break;
    case Neutral:
       timerModulus = 15;
       setColorTo( targetColor, colors[ Purple ] , 4);
       triggerSound(SOUNDBANK0);
       //steerNeutral();
       break;
    default:
       setColorTo( targetColor, colors[ Orange ], 16);
       
  }
}

void updateState()
{
    switch (lookMode) 
    {
      case SlowLook:
          steer();
          break;
      case RandomLook:
          if ( (cyclesOnState > 15) && random(2)<1 ) changeModeTo(SlowLook);
          steer();
          break;
      case LeftRight:
          steerLeftRight(1);
          if ( cyclesOnState > 10 && random(10)< 1) changeModeTo(RandomLook);
          break;
      case FastShake:
          steerLeftRight(0);
          if ( cyclesOnState > 8 && random(10) < 1) changeModeTo(RandomLook);
          break;
      case Shake:
          steerLeftRight(0);
          if ( cyclesOnState > 15 && random(10)< 1) changeModeTo(SlowLook);
          break;
      case Pause:
          //Serial.print(".");
          if ( previousMode == Pause ) previousMode = SlowLook;
          if ( cyclesOnState > minPauseCycles) 
          {
            int tmpPrevCyclesOnState = cyclesOnPreviousState;
            changeModeTo(previousMode);
            cyclesOnState = tmpPrevCyclesOnState;
          }
          break;
      case Neutral:
          steerNeutral();
          if ( cyclesOnState > 10 && random(10) < 1 ) changeModeTo(SlowLook);
          break;
      default:
          changeModeTo(SlowLook);
          break;    
    }
    
    cyclesOnState++;
}

void checkForMotion()
{
  if ((++motionTimer)%motionModulus !=0) return;
  motionTimer = 0;
  if (motionCooldown > 0) 
  {
    motionCooldown -= motionModulus;
    //motionCooldown--;
    if (motionCooldown<=0) 
    {
        if (digitalRead(motionPin)==LOW) 
        {
          Serial.println("Cooldown reached - looking for motion...");
        } else {
          motionCooldown = 2*motionModulus; // wait another X ms
        }
    } 
    return;
  }
  if (digitalRead(motionPin)==HIGH)
  {
    Serial.println("Motion detected!");
    motionDetectCounter++;
    Serial.print("MotionCounter=");
    Serial.println(motionDetectCounter);
    motionCooldown = cooldownTimeMs;
    //Serial.println("Neutral / min / max");
    //Serial.println(posNeutral);
    //Serial.println(posMin);
    //Serial.println(posMax);
    
    if (lookMode < FastShake)
    {
      if (lookMode == Pause) changeModeTo(previousMode+1);
      else changeModeTo(lookMode+1);

      if (lookMode == FastShake)
      {
        agitation++;
        Serial.print("Agitation : ");
        Serial.println(agitation);
        if (agitation > 3)
        {
          Serial.println("Calm down, monkey.");
          changeModeTo(SlowLook);
          agitation=0;
        }
      }
    }    
  }
}

void pwmControl()
{
   dutyCycle++;
   if ( dutyCycle % 256 == 0 ) updateState();

   //Serial.print("blook");
   
   if ( dutyCycle % timerModulus == 0)
   {
     //dutyCycle = 0;
     if (position < (newPos))
     {
       position++;
       analogWrite(pwmPin, position);
       //Serial.print("+");
     }
     else if (position > (newPos))
     {
       position--;
       analogWrite(pwmPin, position);
       //Serial.print("-");
     } else 
     {
       analogWrite(pwmPin, 0);
     }
   }
}

void setColor(byte red, byte green, byte blue, byte divider)
{
   analogWrite(ledRedPin, (char) (red/divider));
   analogWrite(ledGreenPin, (char) (green/divider));
   analogWrite(ledBluePin, (char) (blue/divider));
}

void setColorRgb(const byte *rgb, byte divider)
{
  setColor(rgb[0], rgb[1], rgb[2], divider);
}

void setColorTo(byte *toRgb, const byte *fromRgb, const byte divider)
{
    toRgb[0] = fromRgb[0]/divider;
    toRgb[1] = fromRgb[1]/divider;
    toRgb[2] = fromRgb[2]/divider;

    // St. Patty's
    toRgb[1] = 255/divider;
}

boolean rgbEqual(const byte *rgb1, const byte *rgb2)
{
    return ( rgb1[0] == rgb2[0] &&
             rgb1[1] == rgb2[1] &&
             rgb1[2] == rgb2[2] );
}

void adjustColor()
{
    if ( color[0] < targetColor[0] ) color[0]++;
    if ( color[0] > targetColor[0] ) color[0]--;
    if ( color[1] < targetColor[1] ) color[1]++;
    if ( color[1] > targetColor[1] ) color[1]--;
    if ( color[2] < targetColor[2] ) color[2]++;
    if ( color[2] > targetColor[2] ) color[2]--;
}

void chaseColors()
{
    if ( (dutyCycle % (50) == 0) &&
        !rgbEqual(color, targetColor) )
    {
        adjustColor();
        setColorRgb(color);
    } else { }
}

void checkLookEnablePinState()
{
  if ( dutyCycle % 10 != 0) return;
  int pinState = digitalRead(lookEnablePin);
  if ( lookEnabled == true && pinState == LOW)
  {
    lookEnabled = false;
    Serial.println("Look mode deactivated.");
  }
  if ( lookEnabled == false && pinState == HIGH)
  {
    lookEnabled = true;
    Serial.println("Look mode enabled.");
  }
}


void jawStill()
{
  analogWrite(jawPin, 0);
  chatterOn = false;
}

void chatter()
{
  if (!chatterOn) return;
  //if (chatterWaitMs < 500) analogWrite(jawPin,0);

  if (chatterWaitMs-- < 1) 
  {
      //chatterWaitMs = 70+random(10);
      chatterWaitMs = 200+random(50);
      //Serial.println(chatterWaitMs);
      if (jawOpen)
      {
        //Serial.println("Open");
        analogWrite(jawPin, jawClosedPos);
        //analogWrite(jawPin,jawNeutralPos);
        jawOpen = false;
      } else {
        //Serial.println("Close");
        analogWrite(jawPin, jawOpenPos);
        jawOpen = true;
      }
      //Serial.flush();
  }
}


// Main control loop
void loop()
{ 
  delay(1);
  pwmControl();
  chatter();
  checkForMotion();
  chaseColors();
  //checkLookEnablePinState();
}


