#include <cstring>
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <msgpack.hpp>
#include <iostream>
#include <csignal>
#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <ctime>
#include <stdio.h>
#include <time.h>

int s;

void signalHandler( int signum )
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    nn_close(s);
    // cleanup and close up stuff here  
    // terminate program
   exit(signum);  
}

// Address of our node in Octal format (01,021, etc)
const uint16_t this_node = 00;

struct payload_s {                  // Structure of our payload
  bool red;
  bool green;
};

RF24 radio(22, 0);
RF24Network network(radio);

int main()
{

  s = nn_socket(AF_SP, NN_PAIR);
  // register signal SIGINT and signal handler  
  signal(SIGINT, signalHandler); 
    
  radio.begin();
  network.begin(/*channel*/ 115, /*node address*/ this_node);
  radio.printDetails();

  nn_bind(s, "ipc:///tmp/test.ipc");
  void* buf = NULL;
  int count;
  while (1){
    std::cout << "NetworkUpdate" << std::endl;
    network.update();
    count = nn_recv(s, &buf, NN_MSG, NN_DONTWAIT);
    if(count != -1 && count > 0 ){
      std::cout << "we got a message from elixir" << std::endl;
      uint8_t type = 0;
      std::memcpy(&type, buf, 1);
      std::cout << "type is: " << (int)type << "\n";
      if( type == 42 ){
          nn_close(s);
          return 0;
      } else {
          msgpack::unpacked result;
          msgpack::unpack(&result, (const char*)buf+1, count-1);
          msgpack::object deserialized = result.get();
          std::cout << deserialized << "\n";
          nn_freemsg(buf);
          std::cout << std::flush;
        }
    } 
      std::cout << "if network available" << std::endl;
      if( network.available() ){
        std::cout << "we got a message from the RF network" << std::endl;
        RF24NetworkHeader header;   
        payload_s payloadS;
        network.read(header,&payloadS,sizeof(payloadS));
        printf("Received payload of type: %c : \nRED STATE: %i\nGREEN STATE: %i\n", header.type, payloadS.red, payloadS.green);
      } else {
        std::cout << "network not available" << std::endl;
      }
    std::cout << "delay 1500" << std::endl;
    delay(1500);
    }
    
  }
