//#include <Adafruit_NeoPixel.h>
#include <FastLED.h>

// A:463 + Bod , B:490 + Bod
#define NUMPIXELS 607
#define Pin1 16
#define Pin3 15
#define Pin5 14
#define Pin7 11
#define Pin9 10
#define Pin11 9
#define BodyPin 8

CRGB leds[NUMPIXELS];

#define NumPix1 67
#define NumPix3 102
#define NumPix5 90
#define NumPix7 71
#define NumPix9 69
#define NumPix11 64
#define NumPixBod 144

const int Pin1Start = 0; 
const int Pin3Start = NumPix1;
const int Pin5Start = Pin3Start + NumPix3;
const int Pin7Start = Pin5Start + NumPix5;
const int Pin9Start = Pin7Start + NumPix7;
const int Pin11Start = Pin9Start + NumPix9;
const int BodyStart = Pin11Start + NumPix11;
const int BodyEnd = NUMPIXELS - 1;

//#define LED_PIN 9
// Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

int CurrentRandomNum = 0;

unsigned long CurrentTime;
struct Time{
  unsigned long LastTriggered;
  long Duration;
};
Time NeuronFrames = {0,0};
Time BodyFrame = {0,2};
Time NeuronWait = {0,50};


const int Length = 5;
int SignalWhite[Length];
int BodyWhite[10];

int CurrentBodyState;
bool BodyRun = false;
bool SignalWait;

const int NumIncomingSignals = 2;
const int NumIncomingPaths = 2;
const int NumOutGoingPaths = 4;
const int NumOutGoing = NumIncomingSignals * 2;
const int NumOutGoingSignals = 4;
const int NumSignals = NumIncomingSignals * NumOutGoingPaths;

const int IncomingPathsStart[NumIncomingPaths] = {Pin3Start - 1,Pin5Start - 1, };
const int IncomingPathsEnd[NumIncomingPaths] = {Pin1Start,Pin3Start};
const int OutgoingPathsStart[NumOutGoingPaths] = {Pin5Start,Pin7Start,Pin9Start,Pin11Start};
const int OutgoingPathsEnd[NumOutGoingPaths] = {Pin7Start - 1,Pin9Start - 1,Pin11Start - 1,BodyStart - 1};

struct SignalControl{
  int Start;
  int Current;
  int End;
  bool active;
  bool Incoming;

  void setup(int NeuronStart, int NeuronEnd){
    active = true;
    Start = NeuronStart;
    End = NeuronEnd;
    Current = Start;
  }
  
  void run(){
    if(Start < End){
      Serial.println(Current);
      for(int x=0; x< Length; x++){
        if(Current - x < Start){break;}
        else if(Current - x <= End){
          //strip.setPixelColor(Current - x,SignalWhite[x],SignalWhite[x],SignalWhite[x]);
          leds[Current - x] = CRGB(SignalWhite[x],SignalWhite[x],SignalWhite[x]);
        } 
      }

      if(Current == End){Incoming = true;}

      if(Current < End + Length){Current++;}
      else{
        active = false;
        Start = 0;
        End = 0;
        Current = 0;
      }
    }

    else if(Start > End){
      for(int x=0; x< Length; x++){
        if(Current + x > Start){break;}
        else if(Current + x >= End){
          //strip.setPixelColor(Current + x,SignalWhite[x],SignalWhite[x],SignalWhite[x]);
          leds[Current + x] = CRGB(SignalWhite[x],SignalWhite[x],SignalWhite[x]);
        } 
      }

      if(Current == End){Incoming = true;}

      if(Current > End - Length){Current--;}
      else{
        active = false;
        Start = 0;
        End = 0;
        Current = 0;
        Incoming = true;
      }
    }
  }
};

SignalControl Incoming[NumIncomingSignals];
SignalControl Outgoing[NumOutGoing][NumOutGoingSignals];


//______________________________________________________________________________//
//==============================================================================//
void ColorSetup(){
  for(int x =0; x < Length; x++){
    SignalWhite[x] = 125 - (125*cos(x * (6.29/(Length-1))));
  }

  for(int x =0; x < 10; x++){
    BodyWhite[x] = 125 - (125*cos(x * (3.14/(9))));
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  ColorSetup();

  FastLED.addLeds<WS2812,Pin1,RGB>(leds,Pin1Start,NumPix1);
  FastLED.addLeds<WS2812,Pin3,RGB>(leds,Pin3Start,NumPix3);
  FastLED.addLeds<WS2812,Pin5,RGB>(leds,Pin5Start,NumPix5);
  FastLED.addLeds<WS2812,Pin7,RGB>(leds,Pin7Start,NumPix7);
  FastLED.addLeds<WS2812,Pin9,RGB>(leds,Pin9Start,NumPix9);
  FastLED.addLeds<WS2812,Pin11,RGB>(leds,Pin11Start,NumPix11);
  FastLED.addLeds<WS2812,BodyPin,RGB>(leds,BodyStart,NumPixBod);

  // strip.begin();
  // strip.show();
  // strip.setBrightness(50);
}

void Body(){
  for(int x = 0; x < NumIncomingSignals; x++){
    if(Incoming[x].Incoming == true){
      Incoming[x].Incoming = false;
      BodyRun = true;
      SignalWait = true;
    }
  }

  if((BodyRun == true || SignalWait == true) && CurrentTime >= BodyFrame.Duration + BodyFrame.LastTriggered){
    for(int x = BodyStart; x < BodyEnd; x++){leds[x] = CRGB(BodyWhite[CurrentBodyState],BodyWhite[CurrentBodyState],BodyWhite[CurrentBodyState]);}

    if(SignalWait == true && CurrentBodyState < 9){CurrentBodyState++;}
    else if(SignalWait == true && CurrentBodyState == 9){
      SignalWait = false;
      for(int i = 0; i < NumOutGoing; i++){
        if(Outgoing[i][0].active == false){
          for(int y = 0; y < NumOutGoingSignals; y++){Outgoing[i][y].setup(OutgoingPathsStart[y],OutgoingPathsEnd[y]);}
          break;
        }
      }
    }
    else if(CurrentBodyState > 0){CurrentBodyState--;}
    else if(CurrentBodyState == 0){BodyRun = false;}

    BodyFrame.LastTriggered = CurrentTime;
  }
}

void loop() {
  CurrentTime = millis();

  if(CurrentTime >= NeuronWait.Duration + NeuronWait.LastTriggered){
    CurrentRandomNum = random(0,NumIncomingPaths);
    for(int x = 0; x < NumIncomingSignals; x++){
      if(Incoming[x].active == false){
        Incoming[x].setup(IncomingPathsStart[CurrentRandomNum],IncomingPathsEnd[CurrentRandomNum]);
        break;
      }
    }

    NeuronWait.Duration = random(250,5000);
    NeuronWait.LastTriggered = CurrentTime;
  }

  if(CurrentTime >= NeuronFrames.Duration + NeuronFrames.LastTriggered){
    for(int x=0; x < NumIncomingSignals; x++){
      if(Incoming[x].active == true){Incoming[x].run();}
    }

    for(int x = 0; x < NumOutGoing; x++){
      for(int i = 0; i < NumOutGoingSignals; i++){
        if(Outgoing[x][i].active == true){Outgoing[x][i].run();}
      }
    }

    Body();

    NeuronFrames.LastTriggered = CurrentTime;
  }

  FastLED.show();
  //strip.show();
}
