#include <Arduino.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <stdlib.h>

RF24 radio(8,10);
int i = 0;
const char id[] = "DYN00112.98890600080.25222600";
char ackmag[] = "";
const byte address[][6] = {"00001","00002","00003","00004"};

char CTDcmnd[] = "CTD";
char REcmnd[] = "RESET";
char endtrigger[] = "ENDDYN001";
unsigned long currentMillis;
unsigned long prevMillis;
unsigned long intervalMillis = 1000;
bool tx = false;


bool seeack()
{
  Serial.println("Checking ack");
  radio.startListening();
  delay(50);
  prevMillis = millis();
  currentMillis = prevMillis;
  while(radio.available())
  {
    char text[10] = {0};
    radio.read(&text,sizeof(text));
    if((text[0]==endtrigger[0])&&(text[1]==endtrigger[1])&&(text[2]==endtrigger[2])&&(text[3]==endtrigger[3])&&(text[4]==endtrigger[4])&&(text[5]==endtrigger[5])&&(text[6]==endtrigger[6])&&(text[7]==endtrigger[7])&&(text[8]==endtrigger[8]))
    {
      radio.stopListening();
      delay(50);
      prevMillis = millis();
      currentMillis = prevMillis;
      return true;
    }
  }
  while(!radio.available())
  {
    currentMillis = millis();
    if(currentMillis - prevMillis >= 2000)
    {
      radio.stopListening();
      delay(50);
      return false;
    }
  }
}

void setup() {
  delay(2000);
  while (!Serial);
    Serial.begin(9600);

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(0, address[2]);
  radio.openWritingPipe(address[3]);
  radio.setAutoAck(true);
  radio.enableAckPayload();
  radio.startListening();
  radio.flush_rx();
  radio.flush_tx();
  randomSeed(42);
  //radio.writeAckPayload(1,&id,sizeof(id));
}

void loop() {
  if(i == 0)
  {
    if(radio.available())
    {
      char text[4] = {0};
      radio.read(&text,sizeof(text));
      Serial.println(text);
      if(text[0] == CTDcmnd[0] && text[1] == CTDcmnd[1] && text[2] == CTDcmnd[2])
      {
        i++;
      }
    }
  }
  if(i == 1)
  {
    Serial.println("Responding to CTD");
    radio.stopListening();
    delay(100);
    i++;
  }
  if(i == 2)
  {
    long delaytime = random(100,800);
    delay(delaytime);
    bool tx = radio.write(&id,sizeof(id));
    Serial.println(tx);
    i++;
  }
  if(i == 3)
  {
    Serial.println("Waiting for END");
    radio.startListening();
    delay(100);
    i++;
    prevMillis = millis();
  }
  if(i == 4)
  {
    if(radio.available())
    {
      char text[32] = {0};
      radio.read(&text,sizeof(text));
      Serial.println(text);
      if((text[0]==endtrigger[0])&&(text[1]==endtrigger[1])&&(text[2]==endtrigger[2])&&(text[3]==endtrigger[3])&&(text[4]==endtrigger[4])&&(text[5]==endtrigger[5])&&(text[6]==endtrigger[6])&&(text[7]==endtrigger[7])&&(text[8]==endtrigger[8]))
      {
        Serial.println("Shutting");
        i++;
      }
    }
    if(!radio.available())
    {
      currentMillis = millis();
      if(currentMillis - prevMillis >= intervalMillis)
      {
        Serial.print("Timeout ");
        i = 1;
      }
    }
  }
  if(i == 5)
  {
    Serial.println("Waiting for RESET");
    //radio.startListening();
    //delay(100);
    i++;
  }
  if(i == 6)
  {
    if(radio.available())
    {
      char text[32] = {0};
      radio.read(&text,sizeof(text));
      Serial.println(text);
      if(text[0] == REcmnd[0] && text[1] == REcmnd[1] && text[2] == REcmnd[2] && text[3] == REcmnd[3] && text[4] == REcmnd[4] && text[5] == REcmnd[5])
      {
        Serial.println("Back to Step 1");
        i = 0;
      }
    }
  }
}