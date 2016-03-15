 
 
 /** RF24Mesh_Example_Master.ino by TMRh20
  * 
  * Note: This sketch only functions on -Arduino Due-
  *
  * This example sketch shows how to manually configure a node via RF24Mesh as a master node, which
  * will receive all data from sensor nodes.
  *
  * The nodes can change physical or logical position in the network, and reconnect through different
  * routing nodes as required. The master node manages the address assignments for the individual nodes
  * in a manner similar to DHCP.
  *
  */
  
#include "RF24Mesh/RF24Mesh.h"  
#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <time.h>

using namespace std;

RF24 radio(22,0);  
RF24Network network(radio);
RF24Mesh mesh(radio,network);

struct payload_g {                  // Structure of generic payload 
   char message[20];
};

struct payload_s {                  // Structure of payload outletbox state payload
  bool red;
  bool green;
};

int main(int argc, char** argv) {
  
  // Set the nodeID to 0 for the master node
  mesh.setNodeID(0);
  // Connect to the mesh
  printf("start\n");
  mesh.begin();
  // give the mesh 25ms to get ready..
  delay(25);
  radio.printDetails();

// infinite loop
while(1)
{
  
  // Call network.update as usual to keep the network updated
  mesh.update();

  // In addition, keep the 'DHCP service' running on the master node so addresses will
  // be assigned to the sensor nodes
  mesh.DHCP();
    
  // Check for incoming data from the sensors
  while(network.available()){
     // we got a packet!
//    printf("rcv\n");
    // create a new header struct
    RF24NetworkHeader header;
    // grab header details from packet and store them in header
    network.peek(header);
    // create new vars to store any incoming messages
    uint32_t dat=0;
    uint32_t bat=0;
    payload_s state_payload;
    // handle different message types
    switch(header.type){
      // Display the incoming millis() values from the sensor nodes
      case 'M': network.read(header,&dat,sizeof(dat)); 
                printf("Rcv %u from %i\n",dat,  mesh.getNodeID(header.from_node));
                break;
      // display battery percent from a particular node
      case 'B': network.read(header,&bat,sizeof(bat)); 
                printf("Battery percent from %i: %u\n" ,  mesh.getNodeID(header.from_node), bat);
                break;
      // display state of outlet box
      case 'S': network.read(header,&state_payload,sizeof(state_payload)); 
                printf("Received payload of type: %c from node: %i : \nRED STATE: %i\nGREEN STATE: %i\n", header.type, mesh.getNodeID(header.from_node), state_payload.red, state_payload.green);
                break;
      default:  network.read(header,0,0); 
                printf("Rcv bad type %d from %i\n",header.type, mesh.getNodeID(header.from_node)); 
                break;
    }
  }
// delay 2ms before looping again
delay(2);
  }
return 0;
}

      
      
      
