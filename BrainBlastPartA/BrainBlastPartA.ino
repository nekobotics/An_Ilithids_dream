#include <OctoWS2811.h>

const int SignalLength = 5;
int SignalWhite[SignalLength];
int BodyWhite[10];
int BodyStatus;
bool CellSignalRecieved;

const int numPins = 7;
byte pinList[numPins] = {3, 22, 23, 14, 16, 8, 2};

const int ledsPerStrip = 144;

DMAMEM int displayMemory[ledsPerStrip*6];
int drawingMemory[ledsPerStrip*6];

const int config = WS2811_GRB | WS2811_800kHz;

OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config, numPins, pinList);

const int Pin1Start = 0; 
const int Pin3Start = ledsPerStrip;
const int Pin5Start = Pin3Start + ledsPerStrip;
const int Pin7Start = Pin5Start + ledsPerStrip;
const int Pin9Start = Pin7Start + ledsPerStrip;
const int Pin11Start = Pin9Start + ledsPerStrip;
const int BodyStart = Pin11Start + ledsPerStrip;
const int BodyEnd = BodyStart + ledsPerStrip;

const int NumOutgoing = 4;
const int NumIncoming = 2;

const int IncomingStart[NumIncoming] = {Pin3Start - (1 + (ledsPerStrip - 84)), Pin5Start - 1};
const int IncomingEnd[NumIncoming] = {Pin1Start, Pin3Start};
const int OutgoingStart[NumOutgoing] = {Pin5Start, Pin7Start, Pin9Start, Pin11Start};
const int OutgoingEnd[NumOutgoing] = {Pin5Start + 73, Pin9Start-1, Pin11Start -1, BodyStart-1};

unsigned long CurrentTime;
struct Time{
  unsigned long LastTriggered;
  long Duration;
};
Time NeuronFrames = {0,8};
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
    if(Start > End){
      for(int x=0; x < SignalLength; x++){
        if(Current + x > Start){break;}
        else if(Current + x >= End){leds.setPixel(Current+x,SignalWhite[x],SignalWhite[x],SignalWhite[x]);}
      }

      if(Current != End - SignalLength){Current--;}
      else if(Current == End - SignalLength){active = false;}
    }

    else if (Start < End){
      for(int x=0; x < SignalLength; x++){
        if(Current - x < Start){break;}
        else if(Current - x <= End){leds.setPixel(Current-x,SignalWhite[x],SignalWhite[x],SignalWhite[x]);}
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
    if(FromOtherCell == false){Incoming.setup(IncomingStart[0], IncomingEnd[0]);}
    else if(FromOtherCell == true){Incoming.setup(IncomingStart[1], IncomingEnd[1]);}
  }

  void run(){
    if(SentSignal == false){
      if(Incoming.active == true){Incoming.run();}
      else if(BodyStatus < 10){AtBody = true;}
      else if (BodyStatus == 10){
        AtBody = false;
        SentSignal = true;
        CurrentOutgoing = 0;

        for(int x = 0; x < NumOutgoing; x++){
          if(FromCell == false){Outgoing[x].setup(OutgoingStart[x],OutgoingEnd[x]);}
          else if(OutgoingStart[x] != Pin5Start){Outgoing[x].setup(OutgoingStart[x],OutgoingEnd[x]);}
        }
      }
    }
    else {
      for(int x = 0; x < CurrentOutgoing; x++){if(Outgoing[x].active == true){Outgoing[x].run();}}

      if(CurrentTime >= AxonWait.Duration + AxonWait.LastTriggered && CurrentOutgoing < NumOutgoing){
        CurrentOutgoing++;
        AxonWait.LastTriggered = CurrentTime;
      }

      
      if(Outgoing[0].active == false && FromCell == false){
        digitalWrite(15,HIGH);
        digitalWrite(13,HIGH);
        SignalSendWait.LastTriggered = CurrentTime;
      }
      else if (CurrentTime >= SignalSendWait.LastTriggered + SignalSendWait.Duration && FromCell == false){
        digitalWrite(15,LOW);
        digitalWrite(13,LOW);
      }

      if(Outgoing[0].active == false && Outgoing[1].active == false && Outgoing[2].active == false && Outgoing[3].active == false){active = false;}
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
}

void Body(){
 if(CurrentTime >= BodyHold.Duration + BodyHold.LastTriggered){
    if(BodyStatus >= 0){
      for(int x = BodyStart; x < BodyEnd; x++){leds.setPixel(x,BodyWhite[BodyStatus],BodyWhite[BodyStatus],BodyWhite[BodyStatus]);}
    }
    
    if((Signal[0].AtBody == true || Signal[1].AtBody == true || Signal[2].AtBody == true) && BodyStatus < 10){BodyStatus++;}
    else if(BodyStatus >= 0){BodyStatus--;}

    if(BodyStatus == 10){BodyHold.LastTriggered = CurrentTime;}
  }
}

void loop() {
  // if(Test.active == false){Test.setup(OutgoingStart[0], );}
  // else{Test.run();}

  CurrentTime = millis();

  if(digitalRead(17) == HIGH && CellSignalRecieved == false){
    digitalWrite(13,HIGH);
    //Serial.println("Recieved");
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
    DendriteWait.Duration = random(1000,5000);
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
