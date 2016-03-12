/*
 Update 2014 - TMRh20
 */

/**
 * Simplest possible example of using RF24Network,
 *
 * RECEIVER NODE
 * Listens for messages from the transmitter and prints them out.
 */

//#include <cstdlib>
#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <time.h>
#include <system_error>
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <msgpack.hpp>

/**
 * g++ -L/usr/lib main.cc -I/usr/include -o main -lrrd
 **/
using namespace std;

// CE Pin, CSN Pin, SPI Speed

// Setup for GPIO 22 CE and GPIO 25 CSN with SPI Speed @ 1Mhz
//RF24 radio(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_18, BCM2835_SPI_SPEED_1MHZ);

// Setup for GPIO 22 CE and CE0 CSN with SPI Speed @ 4Mhz
//RF24 radio(RPI_V2_GPIO_P1_15, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_4MHZ); 

// Setup for GPIO 22 CE and CE1 CSN with SPI Speed @ 8Mhz
RF24 radio(22, 0);
RF24Network network(radio);

// Address of our node in Octal format (01,021, etc)
const uint16_t this_node = 00;

// Address of the other node
const uint16_t other_node = 01;

const unsigned long interval = 10000; //ms  // How often to send 'hello world to the other unit

unsigned long last_sent;             // When did we last send?
unsigned long packets_sent;          // How many have we sent already

struct payload_t {                  // Structure of our payload
   char message[20];
};

struct payload_s {                  // Structure of our payload
  bool red;
  bool green;
};

payload_s outlet_box_state;

int main(int argc, char** argv) 
{
  int s = nn_socket(AF_SP, NN_PAIR);
  nn_bind(s, "ipc:///tmp/outlet_box.ipc");
  void* buf = NULL;
  int count;
	// Refer to RF24.h or nRF24L01 DS for settings

	radio.begin();
  bool setRate;
  setRate = radio.setDataRate(RF24_250KBPS);
  std::cout << setRate << "\n";
	delay(5);
	network.begin(/*channel*/ 115, /*node address*/ this_node);
	radio.printDetails();
	
	while(1){

  	network.update();
  	while ( network.available() ) {     // Is there anything ready for us?
      			
  		 	RF24NetworkHeader header;        // If so, grab it and print it out
     			 payload_s payloadS;
    			 network.read(header,&payloadS,sizeof(payloadS));
  			
  			printf("Received payload of type: %c : \nRED STATE: %i\nGREEN STATE: %i\n", header.type, payloadS.red, payloadS.green);
    }

    if((count = nn_recv(s, &buf, NN_MSG, 0)) != -1) {
      uint8_t type = 0;
      std::memcpy(&type, buf, 1);
      std::cout << "type is: " << (int)type << "\n";
      if(type == 42) break;
      
      msgpack::unpacked result;
      msgpack::unpack(&result, (const char*)buf+1, count-1);
      msgpack::object deserialized = result.get();
      std::cout << deserialized << "\n";
      nn_freemsg(buf);
      std::cout << std::flush;
  }
  nn_close(s);
   


		unsigned long now = millis();              // If it's time to send a message, send it!
		if ( now - last_sent >= interval  ){
    			last_sent = now;
    			printf("Sending ..\n");
			payload_t payloadM = { "a message!" };
		        RF24NetworkHeader header(/*to node*/ other_node, 'R');
			bool ok = network.write(header,&payloadM,sizeof(payloadM));
		        if (ok){
		        	printf("ok.\n");
		        }else{ 
      				printf("failed.\n");
  			}
		}
	}
  
	return 0;

}

