#include <OctoWS2811.h>

// A:463 + Bod , B:490 + Bod
// #define Pin1 3
// #define Pin3 22
// #define Pin9 16
// #define Pin11 8
// #define BodyPin 2

const int numPins = 5;
byte pinList[numPins] = {3, 22, 16, 8, 2};

const int ledsPerStrip = 205;

DMAMEM int displayMemory[ledsPerStrip*6];
int drawingMemory[ledsPerStrip*6];

const int config = WS2811_GRB | WS2811_800kHz;

OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config, numPins, pinList);

#define NumPix1 112
#define NumPix3 71 //+ 46
#define NumPix9 165 //+ 73
#define NumPix11 95 //+ 28
#define NumPixBod 205

const int Pin1Start = 0; 
const int Pin3Start = ledsPerStrip;
const int Pin9Start = Pin3Start + ledsPerStrip;
const int Pin11Start = Pin9Start + ledsPerStrip;
const int BodyStart = Pin11Start + ledsPerStrip;
const int BodyEnd = BodyStart + ledsPerStrip;

bool SignalFromAActive;
bool SignalFromA;

int CurrentRandomNum = 0;

unsigned long CurrentTime;
struct Time{
  unsigned long LastTriggered;
  long Duration;
};
Time NeuronFrames = {0,2};
Time BodyFrame = {0,3};
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

const int IncomingPathsStart[NumIncomingPaths] = {Pin11Start};
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
        else if(Current - x <= End){leds.setPixel(Current - x, SignalWhite[x],SignalWhite[x],SignalWhite[x]);} 
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
        else if(Current + x >= End){leds.setPixel(Current + x, SignalWhite[x],SignalWhite[x],SignalWhite[x]);} 
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

  Serial3.begin(9600); // recieve
  Serial4.begin(9600); // transmit

  ColorSetup();

  // FastLED.addLeds<WS2812,Pin1,RGB>(leds,Pin1Start,NumPix1);
  // FastLED.addLeds<WS2812,Pin3,RGB>(leds,Pin3Start,NumPix3);
  // FastLED.addLeds<WS2812,Pin9,RGB>(leds,Pin9Start,NumPix9);
  // FastLED.addLeds<WS2812,Pin11,RGB>(leds,Pin11Start,NumPix11);
  // FastLED.addLeds<WS2812,BodyPin,RGB>(leds,BodyStart,NumPixBod);

  leds.begin();
  leds.show();
}

void Body(){
  for(int x = 0; x < NumIncomingSignals; x++){
    if(Incoming[x].Incoming == true){
      Incoming[x].Incoming = false;
      Incoming[x].active = false;
      BodyRun = true;
      SignalWait = true;
      break;
    }
  }

  if((BodyRun == true) && CurrentTime >= BodyFrame.Duration + BodyFrame.LastTriggered){
    for(int x = BodyStart; x < BodyEnd; x++){leds.setPixel(x,BodyWhite[CurrentBodyState],BodyWhite[CurrentBodyState],BodyWhite[CurrentBodyState]);}

    if(SignalWait == true && CurrentBodyState < 9){CurrentBodyState++;}
    else if(SignalWait == true && CurrentBodyState == 9){
      SignalWait = false;
      for(int i = 0; i < NumOutGoing; i++){
        if(Outgoing[i][0].active == false){
          if(SignalFromA == false){Serial4.println(1);}
          else{SignalFromA = false;}
          for(int y = 0; y < NumOutGoingSignals; y++){
            Outgoing[i][y].setup(OutgoingPathsStart[y],OutgoingPathsEnd[y]);
          }
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
  if(Serial3.read() == 1){
    for(int x = 0; x < NumIncomingSignals; x++){
      if(Incoming[x].active == false){
        Incoming[x].Incoming = true;
        Incoming[x].active = true;
        break;
      }
    }
    Serial3.clear();
    SignalFromA = true;
  }
  else{SignalFromA = false;}

  if(CurrentTime >= NeuronWait.Duration + NeuronWait.LastTriggered){
    for(int x = 0; x < NumIncomingSignals; x++){
      if(Incoming[x].active == false){
        Incoming[x].setup(IncomingPathsStart[0],IncomingPathsEnd[0]);
        break;
      }
    }
    NeuronWait.Duration = random(1000,5000);
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

  leds.show();
}
