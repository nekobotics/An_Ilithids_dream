#include <OctoWS2811.h>

const int SignalLength = 5;
float SignalWhite[SignalLength];
int BodyWhite[10];
int BodyStatus;
bool CellSignalRecieved;

const int numPins = 5;
byte pinList[numPins] = {3, 22, 16, 8, 2};

const int ledsPerStrip = 205;

DMAMEM int displayMemory[ledsPerStrip*6];
int drawingMemory[ledsPerStrip*6];

const int config = WS2811_GRB | WS2811_800kHz;

OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config, numPins, pinList);

const int Pin1Start = 0; 
const int Pin3Start = ledsPerStrip;
const int Pin9Start = Pin3Start + ledsPerStrip;
const int Pin11Start = Pin9Start + ledsPerStrip;
const int BodyStart = Pin11Start + ledsPerStrip;
const int BodyEnd = BodyStart + ledsPerStrip;

const int Pin1End = Pin1Start + 112;
const int Pin3End = Pin3Start + 63;
const int Pin9End = Pin9Start + 137;
const int Pin11End = Pin11Start + 95;

const int NumOutgoing = 3;
const int NumIncoming = 1;

const int IncomingStart[NumIncoming] = {Pin9End};
const int IncomingEnd[NumIncoming] = {Pin9Start};
const int OutgoingStart[NumOutgoing] = {Pin1Start,Pin3Start,Pin11Start};
const int OutgoingEnd[NumOutgoing] = {Pin1End,Pin3End,Pin11End};

unsigned long CurrentTime;
struct Time{
  unsigned long LastTriggered;
  long Duration;
};
Time NeuronFrames = {0,2};
Time BodyFrame = {0,10};
Time BodyHold = {0,150};
Time DendriteWait = {0,0};

struct SignalControl{
  int Start;
  int Current;
  int End;
  bool active;

  void setup(int StartPoint, int EndPoint){
    active = true;
    Start = StartPoint;
    End = EndPoint;
    Current = StartPoint;
  }

  void run(){
    float FadeStart = (abs(Start - End))/2;

    if(Start > End){
      float FadeIn = ((Start - Current)/FadeStart);
      for(int x=0; x < SignalLength; x++){
        if(Current > Start - FadeStart){
          if(Current + x > Start){break;}
          else if(Current + x >= End){leds.setPixel(Current+x,SignalWhite[x] * FadeIn,SignalWhite[x] * FadeIn,SignalWhite[x] *FadeIn);}
        }
        else{
          if(Current + x > Start){break;}
          else if(Current + x >= End){leds.setPixel(Current+x,SignalWhite[x],SignalWhite[x],SignalWhite[x]);}
        }
      }

      if(Current != End - SignalLength){Current--;}
      else if(Current == End - SignalLength){active = false;}
    }

    else if (Start < End){
      float FadeOut = ((End - Current)/FadeStart);

      for(int x=0; x < SignalLength; x++){
        if(Current > End - FadeStart){
          if(Current - x < Start){break;}
          else if(Current - x <= End && FadeOut >= 0){leds.setPixel(Current-x,SignalWhite[x] * FadeOut,SignalWhite[x] * FadeOut,SignalWhite[x] * FadeOut);}
        }
        else{
          if(Current - x < Start){break;}
          else if(Current - x <= End){leds.setPixel(Current-x,SignalWhite[x],SignalWhite[x],SignalWhite[x]);}
        }
      }

      if(Current != End + SignalLength){Current++;}
      else if(Current == End + SignalLength){active = false;}
    }
  }
};

SignalControl Test;

const int NumSignals = 3;

struct SignalSequencer{
  SignalControl Incoming;
  SignalControl Outgoing[NumOutgoing];
  Time AxonWait = {0,100};
  Time SignalSendWait = {0,NeuronFrames.Duration * 2};
  int CurrentOutgoing;
  bool active;
  bool AtBody;
  bool FromCell;
  bool SentSignal;

  void setup(bool FromOtherCell){
    active = true;
    SentSignal = false;
    FromCell = FromOtherCell;
    if(FromOtherCell == false){Incoming.setup(Pin9End, Pin9Start);}
  }

  void run(){
    if(SentSignal == false){
      if(Incoming.active == true){Incoming.run();}
      else if(BodyStatus < 10){AtBody = true;}
      else if (BodyStatus == 10){
        AtBody = false;
        SentSignal = true;
        CurrentOutgoing = 0;

        if(FromCell == false){
          digitalWrite(15,HIGH);
          digitalWrite(13,HIGH);
          SignalSendWait.LastTriggered = CurrentTime;
          //Serial.println("Send");
        }

        for(int x = 0; x < NumOutgoing; x++){
          Outgoing[x].setup(OutgoingStart[x],OutgoingEnd[x]);
        }
      }
    }
    else {
      for(int x = 0; x < CurrentOutgoing; x++){if(Outgoing[x].active == true){Outgoing[x].run();}}
      
      if(CurrentTime >= SignalSendWait.Duration + SignalSendWait.LastTriggered && FromCell == false){
        digitalWrite(15,LOW);
        digitalWrite(13,LOW);
      }

      if(CurrentTime >= AxonWait.Duration + AxonWait.LastTriggered && CurrentOutgoing < NumOutgoing){
        CurrentOutgoing++;
        AxonWait.LastTriggered = CurrentTime;
      }

      if(Outgoing[0].active == false && Outgoing[1].active == false && Outgoing[2].active == false){active = false;}
    }
  }
};

SignalSequencer Signal[NumSignals];

void ColorSetup(){
  for(int x =0; x < SignalLength; x++){SignalWhite[x] = 125 - (125*cos(x * (6.29/(SignalLength-1))));}
  for(int x =0; x < 10; x++){BodyWhite[x] = 125 - (125*cos(x * (3.14/(9))));}
}

void setup() {
  ColorSetup();
  
  Serial.begin(9600);

  pinMode(17,INPUT_PULLDOWN);
  pinMode(15,OUTPUT);
  pinMode(13,OUTPUT);

  leds.begin();
  leds.show();

  Test.setup(Pin9Start, Pin11Start);
}

void Body(){
  if(CurrentTime >= BodyHold.Duration + BodyHold.LastTriggered){
    if(BodyStatus >= 0){
      for(int x = BodyStart; x < BodyEnd; x++){leds.setPixel(x,BodyWhite[BodyStatus],BodyWhite[BodyStatus],BodyWhite[BodyStatus]);}
    }
    
    if((Signal[0].AtBody == true || Signal[1].AtBody == true || Signal[2].AtBody == true) && BodyStatus < 10){BodyStatus++;}
    else if(BodyStatus >= 0){
      BodyStatus--;}

    if(BodyStatus == 10){BodyHold.LastTriggered = CurrentTime;}
  }
}

void TestSig(){

  if(Test.active == true){Test.run();}
}

void Main() {
  CurrentTime = millis();

 if(digitalRead(17) == HIGH && CellSignalRecieved == false){
    digitalWrite(13,HIGH);
    for(int x = 0; x < NumSignals; x++){
      if(Signal[x].active == false){
        Signal[x].setup(true);
        CellSignalRecieved = true;
        break;
      }
    }
  }
  else if(digitalRead(17) == LOW && CellSignalRecieved == true){
    CellSignalRecieved = false;
    digitalWrite(13,LOW);
  }

  if(CurrentTime >= DendriteWait.Duration + DendriteWait.LastTriggered){
    for(int x = 0; x < NumSignals; x++){
      if(Signal[x].active == false){
        Signal[x].setup(false);
        break;
      }
    }
    DendriteWait.Duration = random(500,2000);
    DendriteWait.LastTriggered = CurrentTime;
  }

  if(CurrentTime >= BodyFrame.Duration + BodyFrame.LastTriggered){
    Body();
    BodyFrame.LastTriggered = CurrentTime;
  }

  if(CurrentTime >= NeuronFrames.Duration + NeuronFrames.LastTriggered){
    for(int x=0; x < NumSignals; x++){if(Signal[x].active == true){Signal[x].run();}}
  }

  leds.show();
}

void loop(){
  Main();
  //TestSig();
}
