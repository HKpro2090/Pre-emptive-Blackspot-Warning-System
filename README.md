# Pre-emptive Blackspot Warning System

## Introduction
Unmarked Road Hazards or black spots are a major cause of road accidents and fatalities. These hazards which are not properly marked like unmarked speed breakers; barricades or road diversions pose a major risk to road users. Existing reflective paint or markers suffer from environmental pitfalls such as poor visibility in rain, fog, or low light conditions. Regular wear and tear also reduces the visibility and reflective capability of these road hazard markings. To address these drawbacks, we propose a structured alert system that identifies the hazards and intimates the public based on their proximity. 

In this proposed system we have implemented geotagged IoT enabled NRF mesh nodes which update the node location to a real-time cloud database (Google FireBase) via a coordinator node and a receiver car node. These black spot node locations are then retrieved and displayed to the public based on their proximity to their current location via a mobile app which also provides alerts in real-time.

This repo deals with the Wireless Sensor Network part of the project. Will update the links to the other parts soon.

## Basic Working Diagram

![Proposed Block Diagram](https://github.com/HKpro2090/Pre-emptive-Blackspot-Warning-System/blob/main/Proposed%20Block%20Diagram.png?raw=true)

## Working
The mesh communication starts with Car Node sending LFC instruction (Looking For Coordinator). This instruction helps the Car Node to identify the nearby Coordinator Node. Once the LFC instruction is received by a Coordinator Node it acknowledges back with its Node ID and location. The acknowledgement is received by the Car Node, and it is saved in a linked list. After this, the Coordinator Node changes to a different communication pipe and talks to the nearby Dynamic Nodes to gather the information and sends it to the Car Node. The coordinator node sends CTD Instruction (Coordinator to Dynamic) which helps the node to identify the nearby Dynamic Nodes. Once the Dynamic Nodes receive the CTD instruction, they acknowledge back with their Node ID and location to the Coordinator Node. 

Once the data of all the Dynamic Nodes are received, the Coordinator Node sends the information to the Car Node one by one and sends a DONE instruction to denote the end of communication between the coordinator node and the Car Node. Later the Dynamic and the Coordinator Nodes return to their initial states. Then the Car Node breaks down the messages and establishes communication with FireBase via Wi-Fi. Then the data of the nearby nodes and locations are updated in the FireBase. After successful communication, the Car Node returns to sending LFC to look for other coordinators. The blackspot coordinates updated in the FireBase database are synced to an Android Application automatically.

## Hardware Used
- Node 32s
- nRF24L01+ PA LNA Wireless Transceiver Module with External Antenna
- Arduino Uno R3


## Libraries Used
- RF24 (https://github.com/nRF24/RF24)
- Firebase - ESP32 (https://github.com/mobizt/Firebase-ESP32)

## Files and Folders Description
Car Node - Contains the source code for the Car Node. This code is developed for ESP32 which has WiFi and Blutooth inbuilt. This code uses the WiFi to send data to Firebase.
Coordinate Node - Contains the source code for Coordinator Node. This Code is developed for Arduino Uno.
Dynamic Node - Contains the source code for Dynamic Node. This code is developed for Arduino Uno.
