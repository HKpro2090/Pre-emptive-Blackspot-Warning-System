#include <Arduino.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <EEPROM.h>
#include <stdlib.h>

#include <WiFi.h>
#include <FirebaseESP32.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#define WIFI_SSID "**********"
#define WIFI_PASSWORD "*********************"
#define API_KEY "**********************"
#define DATABASE_URL "************************"
#define USER_EMAIL "************************"
#define USER_PASSWORD "*****************"
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
int count = 0;
int c = 1;
RF24 radio(17, 5);  // CE, CSN
int i = 0;

void FirebaseStartup()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
}

void FirebaseNoOfHazards(String noofh)
{
  if (Firebase.ready())
  {
    Serial.printf("Sending Data... %s\n", Firebase.setString(fbdo, "/NoOfPoints", noofh) ? "ok" : fbdo.errorReason().c_str());
  }
}

void FirebaseHazardstype(String typeh)
{
  String nameid = "/loc";
  nameid = nameid + String(c) + "type";
  if (Firebase.ready())
  {
    Serial.printf("Sending Hazard Data... %s\n", Firebase.setString(fbdo, nameid, typeh) ? "ok" : fbdo.errorReason().c_str());
  }
  //c++;
}

void FirebaseHazardsloac(String latt,String longt, String id)
{
  String nameid = "/loc";
  nameid = nameid + String(c);
  String loca = latt + "," + longt + "," + id;
  if (Firebase.ready())
  {
    Serial.printf("Sending Location Data... %s\n", Firebase.setString(fbdo, nameid, loca) ? "ok" : fbdo.errorReason().c_str());
  }
  c++;
}

typedef struct staticnodedata{
  char id[7];
  char latt[12];
  char longt[12];
  struct staticnodedata * link;
}staticnode;

staticnode *front = NULL;
staticnode *rear = NULL;

const char id[] = "CAR001";
const byte address[][6] = {"00001","00002","00003","00004"};
const char LFCcmnd[] = "LFC";
const char endtrigger[] = "END";
const char DONEcmd[] = "DONE";

unsigned long currenttime;
unsigned long prevtime;


void enqueue(char* msg)
{
  count++;
  staticnode* newnode;
  newnode = (staticnode*)malloc(sizeof(staticnode));
  for(int i = 0;i < 6;i++)
    newnode->id[i] = *(msg+i);
  newnode->id[6] = '\0';
  int k = 0;
  for(int i = 6;i < 17;i++)
  {
    newnode->latt[k] = *(msg+i);
    k++;
  }
  newnode->latt[11] = '\0';
  k = 0;
  for(int i = 18;i < 29;i++)
  {
    newnode->longt[k] = *(msg+i);
    k++;
  }
  newnode->longt[11] = '\0';
  newnode->link=NULL;
  if(rear == NULL)
  {
    front = newnode;
    rear = newnode;
  }
  else
  {
    rear->link=newnode;
    rear = newnode;
  }
}

void dequeue()
{
  staticnode* temp = front;
  if(front == NULL)
    return;
  temp = front;
  if(front == rear)
    front = rear = NULL;
  else
    front = front->link;
  String id1 = String(temp->id);
  String lat1 = String(temp->latt);
  String lon1 = String(temp->longt);
  FirebaseHazardsloac(lat1,lon1,id1);
  free(temp);
}

void printout()
{
  staticnode* temp = front;
  if(front == NULL)
  {
    Serial.println("Empty");
  }
  else
  {
    while(temp != NULL)
    {
      Serial.print("ID = ");
      Serial.print(temp->id);
      Serial.print(" Lat = ");
      Serial.print(temp->latt);
      Serial.print(" Long = ");
      Serial.println(temp->longt);
      temp = temp->link;
    }
  }
}


void setup() 
{
  while (!Serial);
    Serial.begin(9600);
  radio.begin();
    radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.openWritingPipe(address[0]);
  radio.setAutoAck(true);
  radio.enableAckPayload();
  radio.enableDynamicPayloads();
  radio.stopListening();
  FirebaseStartup();
  delay(5000);
}

void loop() 
{
  currenttime = millis();
  if(i == 0)
  {
    //Serial.println("Writing");
    bool t;
    t = radio.write(LFCcmnd, sizeof(LFCcmnd));
    if(t)
    {
      //Serial.println(t);
      if(radio.isAckPayloadAvailable())
      {
        char ackrec[32] = {0};
        radio.read(&ackrec,sizeof(ackrec));
        Serial.println(ackrec);
        enqueue(ackrec);
        radio.write(&DONEcmd,sizeof(DONEcmd));
        i++;
      }
    }
  }
  if(i == 1)
  {
    Serial.println("Recieving Data");
    radio.openReadingPipe(0,address[1]);
    radio.startListening();
    delay(100);
    i++;
  }
  if(i == 2)
  {
    if(radio.available())
    {
      char text[32] = {0};
      radio.read(&text,sizeof(text));
      Serial.println(text);
      if(text[0] == DONEcmd[0] && text[1] == DONEcmd[1] && text[2] == DONEcmd[2] && text[3] == DONEcmd[3])
      {
        i++;
      }
      else
      {
        enqueue(text);
      }
    }
  }
  if(i == 3)
  {
    Serial.println("printing and Sending the data");
    printout();
    FirebaseNoOfHazards(String(count));
    for(int j = 1;j <= count;j++)
    {
      if(j == 1)
      {
        FirebaseHazardstype("Pot Holes");
      }
      if(j == 2)
      {
        FirebaseHazardstype("Speed Breaker");
      }
      dequeue();
    }
    i++;
  }
  if(i == 4)
  {
    radio.closeReadingPipe(0);
    radio.openWritingPipe(address[0]);
    radio.setAutoAck(true);
    radio.enableAckPayload();
    radio.stopListening();
    delay(100);
  }
}
