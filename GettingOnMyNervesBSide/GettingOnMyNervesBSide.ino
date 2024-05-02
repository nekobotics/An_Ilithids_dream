#include <FastLED.h>

// A:463 + Bod , B:490 + Bod
#define NUMPIXELS 650
#define Pin1 15
#define Pin3 14
#define Pin9 11
#define Pin11 10
#define BodyPin 9
// #define IncomingPin 22
// #define OutgoingPin 21

CRGB leds[NUMPIXELS];

#define NumPix1 142
#define NumPix3 79
#define NumPix9 167
#define NumPix11 127
#define NumPixBod 144

const int Pin1Start = 0; 
const int Pin3Start = NumPix1;
const int Pin9Start = Pin3Start + NumPix3;
const int Pin11Start = Pin9Start + NumPix9;
const int BodyStart = Pin11Start + NumPix11;
const int BodyEnd = NUMPIXELS - 1;

bool SignalFromAActive;
bool SignalFromA;

int CurrentRandomNum = 0;

unsigned long CurrentTime;
struct Time{
  unsigned long LastTriggered;
  long Duration;
};
Time NeuronFrames = {0,0};
Time BodyFrame = {0,2};
Time NeuronWait = {0,50};
Time CellASignalWait = {0,1};


const int Length = 5;
int SignalWhite[Length];
int BodyWhite[10];

int CurrentBodyState;
bool BodyRun = false;
bool SignalWait;

const int NumIncomingSignals = 2;
const int NumIncomingPaths = 2;
const int NumOutGoingPaths = 4;
const int NumOutGoing = NumIncomingPaths * 2;
const int NumOutGoingSignals = 4;
const int NumSignals = NumIncomingSignals * NumOutGoingPaths;

const int IncomingPathsStart[NumIncomingPaths] = {Pin11Start - 1};
const int IncomingPathsEnd[NumIncomingPaths] = {Pin9Start};
const int OutgoingPathsStart[NumOutGoingPaths] = {Pin1Start, Pin3Start, Pin11Start};
const int OutgoingPathsEnd[NumOutGoingPaths] = {Pin3Start - 1,Pin9Start - 1, BodyStart-1};

struct SignalControl{
  int Start;
  int Current;
  int End;
  bool active;
  bool Incoming;
  bool Ready;

  void setup(int NeuronStart, int NeuronEnd){
    active = true;
    Start = NeuronStart;
    End = NeuronEnd;
    Current = Start;
  }
  
  void run(){
    if(Start < End){
      for(int x=0; x< Length; x++){
        if(Current - x < Start){break;}
        else if(Current - x <= End){leds[Current - x] = CRGB(SignalWhite[x],SignalWhite[x],SignalWhite[x]);} 
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
        else if(Current + x >= End){leds[Current + x] = CRGB(SignalWhite[x],SignalWhite[x],SignalWhite[x]);} 
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
  Serial.begin(9600);

  Serial5.begin(9600); // recieve
  Serial4.begin(9600); // transmit

  // pinMode(IncomingPin,INPUT);
  // pinMode(OutgoingPin,OUTPUT);

  ColorSetup();

  FastLED.addLeds<WS2812,Pin1,RGB>(leds,Pin1Start,NumPix1);
  FastLED.addLeds<WS2812,Pin3,RGB>(leds,Pin3Start,NumPix3);
  FastLED.addLeds<WS2812,Pin9,RGB>(leds,Pin9Start,NumPix9);
  FastLED.addLeds<WS2812,Pin11,RGB>(leds,Pin11Start,NumPix11);
  FastLED.addLeds<WS2812,BodyPin,RGB>(leds,BodyStart,NumPixBod);
}

void Body(){
  for(int x = 0; x < NumIncomingSignals; x++){
    if(Incoming[x].Incoming == true){
      Incoming[x].Incoming = false;
      BodyRun = true;
      SignalWait = true;
      break;
    }
  }

  if((BodyRun == true || SignalWait == true) && CurrentTime >= BodyFrame.Duration + BodyFrame.LastTriggered){
    for(int x = BodyStart; x < BodyEnd; x++){leds[x] = CRGB(BodyWhite[CurrentBodyState],BodyWhite[CurrentBodyState],BodyWhite[CurrentBodyState]);}

    if(SignalWait == true && CurrentBodyState < 9){CurrentBodyState++;}
    else if(SignalWait == true && CurrentBodyState == 9){
      SignalWait = false;

      if(SignalFromA == false){
        Serial4.println(1);
      }
      else{SignalFromA = false;}

      for(int i = 0; i < NumOutGoing; i++){
        if(Outgoing[i][0].active == false && Outgoing[i][1].active == false && Outgoing[i][2].active == false && Outgoing[i][3].active == false){
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
  Serial5.read();
  if(Serial5.available() == true){
    Serial.println("activated");
    for(int x = 0; x < NumIncomingSignals; x++){
      if(Incoming[x].active == false){
        Incoming[x].Incoming = true;
        break;
      }
    }
    Serial5.clear();
    SignalFromA = true;
  }

  if(CurrentTime >= NeuronWait.Duration + NeuronWait.LastTriggered){
    for(int x = 0; x < NumIncomingSignals; x++){
      if(Incoming[x].active == false){
        Incoming[x].setup(IncomingPathsStart[CurrentRandomNum],IncomingPathsEnd[CurrentRandomNum]);
        break;
      }
    }
    NeuronWait.Duration = random(250,5000);
    //NeuronWait.Duration = random(5000,10000);

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

    NeuronFrames.LastTriggered = CurrentTime;
  }

  Body();

  FastLED.show();
}
