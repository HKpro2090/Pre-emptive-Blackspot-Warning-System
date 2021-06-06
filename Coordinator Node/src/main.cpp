#include <Arduino.h>
#include <avr/sleep.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <EEPROM.h>


RF24 radio(8, 10);  // CE, CSN

const byte address[][6] = {"00001","00002","00003","00004"};
#define interruptPin 2
#define GLOWLED 3
int i = 0;
const char LFCcmnd[] = "LFC";
const char CTDcmnd[] = "CTD";
const char resetcmd[] = "RESET";
const char DONEcmd[] = "DONE";
const char id = "STA001";
const char ackmag[] = "STA00112.99050100080.24990600";
char endtrigger[] = "ENDDYN001";

bool sending = true;
unsigned long currentMillis;
unsigned long prevMillis;
unsigned long intervalMillis = 5000;
int timetry = 1;
bool timeout = false;
bool content = true;
char t[14] = {0};

typedef struct staticnodecache{
  char id[30];
  struct staticnodecache * link;
}staticcache;

staticcache *front = NULL;
staticcache *rear = NULL;


void enqueue(char* msg)
{
  staticcache* newnode;
  newnode = (staticcache*)malloc(sizeof(staticcache));
  for(int i = 0;i < 30;i++)
  {
    //if(isalnum(msg[i]))
      newnode->id[i] = msg[i];
  }
  newnode->link = NULL;
  if(rear == NULL)
  {
    front = newnode;
    rear = newnode;
  }
  else
  {
    rear->link = newnode;
    rear = newnode;
  }
}

String dequeue()
{
  staticcache* temp = front;
  if(front == NULL)
  {
    return;
  }
  if(front == rear)
  {
    //Serial.println("Last Item");
    front = rear = NULL;
  }
  else
    front = front->link;
    
  //Serial.println(temp->id);
  String s = String(temp->id);
  free(temp);
  return s;
}

void msgretry()
{
  radio.stopListening();
  delay(60);
  radio.write(&CTDcmnd,sizeof(CTDcmnd));
  radio.startListening();
  delay(50);
}

void acksend(char *text)
{
  for(int i = 3;i < 9;i++)
  {
    endtrigger[i] = text[i-3];
  }
  radio.stopListening();
  delay(60);
  radio.write(&endtrigger,sizeof(endtrigger));
  delay(30);
  radio.startListening();
  delay(50);
}

void setup()
{
  //pinMode(3,OUTPUT);
  //analogWrite(3,127);
  delay(2000);
  pinMode(interruptPin,INPUT_PULLUP);
  while (!Serial);
    Serial.begin(9600);
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(0, address[0]);
  //radio.setAutoAck(true);
  radio.enableAckPayload();
  radio.writeAckPayload(0,&ackmag,sizeof(ackmag));
  radio.enableDynamicPayloads();
  radio.startListening();
  radio.flush_rx();
  radio.flush_tx();
}

void loop()
{
  if(i == 0)
  {
    if(radio.available())
    {
      char text[32] = {0};
      radio.read(&text, sizeof(text));
      Serial.println(text);
      if((text[0]==LFCcmnd[0])&&(text[1]==LFCcmnd[1])&&(text[2]==LFCcmnd[2]))
      {
        Serial.println(F("YES"));
        radio.writeAckPayload(0,&ackmag,sizeof(ackmag));
      }
      if((text[0] == DONEcmd[0]) && (text[1] == DONEcmd[1]) && (text[2] == DONEcmd[2]) && (text[3] == DONEcmd[3]))
      {
        i++;
      }
    }
  }
  if(i == 1)
  {
    radio.closeReadingPipe(0);
    radio.openReadingPipe(0,address[3]);
    radio.openWritingPipe(address[2]);
    i++;
  }
  if(i == 2)
  {
    Serial.println(F("Writing CTD"));
    radio.stopListening();
    delay(100);
    i++;
  }
  if(i == 3)
  {
    bool tx = radio.write(&CTDcmnd,sizeof(CTDcmnd));
    Serial.println(tx);
    delay(100);
    i++;
  }
  if(i == 4)
  {
    Serial.println(F("Listening to CTD"));
    radio.startListening();
    delay(100);
    i++;
    prevMillis = millis();
  }
  if(i == 5)
  {
    if(radio.available())
    {
      char text[32] = {0};
      radio.read(&text,sizeof(text));
      Serial.println(text);
      enqueue(text);
      prevMillis = millis();
      timetry = 1;
      i++;
    }
    if(!radio.available())
    {
      currentMillis = millis();
      if(currentMillis - prevMillis >= intervalMillis)
      {
        Serial.print(F("Timeout "));
        Serial.println(timetry);
        timetry++;
        if(timetry > 3)
        {
          i = 8;
        }
        else
        {
          i = 2;
        }
      }
    }
  }
  if(i == 6)
  {
    Serial.println(F("Sending END"));
    radio.stopListening();
    delay(100);
    i++;
  }
  if(i == 7)
  {
    radio.write(&endtrigger,sizeof(endtrigger));
    delay(50);
    i = 4;
  }
  if(i == 8)
  {
    Serial.println(F("Writing RESET"));
    radio.stopListening();
    delay(100);
    i++;
  }
  if(i == 9)
  {
    radio.write(&resetcmd,sizeof(resetcmd));
    i++;
  }
  if(i == 10)
  {
    Serial.println(F("Sending Data to Car"));
    radio.closeReadingPipe(0);
    radio.openWritingPipe(address[1]);
    radio.stopListening();
    delay(100);
    i++;
  }
  if(i == 11)
  {
    while(front != NULL)
    {
      String t = dequeue();
      //Serial.println(t);
      char t1[30] = {0};
      t.toCharArray(t1,sizeof(t1));
      Serial.println(t1);
      radio.write(&t1,sizeof(t1));
      delay(50);
    }
    i++;
  }
  if(i == 12)
  {
    Serial.println(F("Sending DONE"));
    radio.write(&DONEcmd,sizeof(DONEcmd));
    delay(100);
    i++;
  }
  if(i == 13)
  {
    //Serial.println(dequeue());
    //radio.closeReadingPipe(0);
    radio.openReadingPipe(0,address[0]);
    radio.writeAckPayload(0,&ackmag,sizeof(ackmag));
    radio.startListening();
    delay(100);
    Serial.println(F("Back to step 1"));
    i = 0;
  }
}
