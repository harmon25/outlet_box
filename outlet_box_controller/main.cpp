#include <cstring>
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <istream>
#include <streambuf>
#include <string>
#include <iostream>
#include <csignal>
#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <ctime>
#include <stdio.h>
#include <time.h>
#include <json/json.h>

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
const uint16_t this_node = 00; // MASTER NODE 

struct payload_s {                  // Structure of our payload from outlet_box
  bool red;
  bool green;
};

struct payload_json {                  // Structure of our payload
   char msg[24];
};


RF24 radio(22, 0);
RF24Network network(radio);

int main()
{
  s = nn_socket(AF_SP, NN_PAIR);
  // register signal SIGINT and signal handler  
  signal(SIGINT, signalHandler); 
    
  radio.begin();
  delay(5);
  network.begin(/*channel*/ 115, /*node address*/ this_node);
  radio.printDetails();
  nn_connect(s, "ipc:///tmp/test.ipc");
  void* buf = NULL;
  int count;
  Json::StyledWriter styledWriter;
  Json::Reader reader;
  Json::FastWriter fastWriter;
  Json::CharReaderBuilder builder;
  while (1){
    network.update();
    count = nn_recv(s, &buf, NN_MSG, NN_DONTWAIT); //check if we have any messages from elixir
    if(count != -1 && count > 0 )
    {  // if there is not an error and count > 0 we have a message from elixir
      std::string rawjsonMsg = (const char*)buf; 
      nn_freemsg(buf);
      Json::Value jsonMessage;
      if (reader.parse(rawjsonMsg, jsonMessage))
      {
        int msgType = jsonMessage.get("type", 1).asInt();
        uint16_t to_node_id = jsonMessage.get("node_id", 012).asInt();
        std::cout << "Pretty request: "  << std::endl;
        std::cout << styledWriter.write(jsonMessage) << std::endl;
        std::cout << "msg type: " <<  msgType << std::endl;
        std::cout << "to_node_id: " << to_node_id << std::endl;
        std::cout << "Msg: " << std::endl;
        std::cout << jsonMessage.get("msg", "ok").asCString() << std::endl;
        if(network.is_valid_address(to_node_id)){
          std::cout << "valid address! : " << to_node_id << std::endl;
          std::string strMsg = jsonMessage.get("msg", "ok").asString();
          payload_json payload;
          strcpy(payload.msg, strMsg.data());
          std::cout << "msg from struct: " << std::endl;
          std::cout << payload.msg << std::endl;
          RF24NetworkHeader header(/*to node*/ to_node_id , /*msg type, 0-255 */ (unsigned char)msgType); //create RF24Network header
          if (network.write(header,&payload,sizeof(payload))){
            std::cout << "Sent :" << payload.msg << " to node: " <<  to_node_id << " of type: " << msgType << std::endl; 
          } else { 
              std::cout << "failed sending message to node: " <<  to_node_id << " of type: "  << msgType << std::endl; 
          }      
        } else {
           std::cout << "invalid address :(  " << to_node_id << std::endl;
        }  
        std::cout << std::flush; 
      }
           
    }
    if( network.available() ){
      int payloadSize = 24;
      RF24NetworkHeader header;   
      payload_json payloadJ;
      Json::Value jsonRFM;

      //uint16_t this_msg_type = network.peek(header); 
      network.read(header,&payloadJ, payloadSize);
      //printf("Received payload of type: %c : \nRED STATE: %i\nGREEN STATE: %i\n", header.type, payloadS.red, payloadS.green);
      jsonRFM["from_node"] = header.from_node;
      jsonRFM["type"] = header.type;
      std::cout << "from node: " <<  jsonRFM["from_node"] << std::endl;
      std::cout << "msg type: " << jsonRFM["type"] << std::endl;
      std::cout << "Msg: " << std::endl;
      //std::cout << jsonRFM.get("msg", "[]").asString() << std::endl;
      Json::Value parsedFromString;
      
      std::istringstream iss;
      iss.str (payloadJ.msg);
      builder["collectComments"] = false;
      std::string errs;
      bool parsingSuccessful = Json::parseFromStream(builder, iss, &parsedFromString, &errs);
      if (parsingSuccessful)
      {
        jsonRFM["msg"] = parsedFromString;
        std::cout << styledWriter.write(jsonRFM) << std::endl;
      } else {
        std::cout << errs << std::endl;
      }
      std::string str_for_elixir = fastWriter.write(jsonRFM);
      std::cout << "string to send to elixir:" << std::endl;
      std::cout << str_for_elixir << std::endl;
      void *buf = nn_allocmsg(sizeof("test str"), 0);
      memcpy (buf, "test str", 0 );
      int nbytesent = nn_send(s, &buf, NN_MSG, NN_DONTWAIT);
      std::cout << "bytes sent to elixir: " << std::endl;
      std::cout << nbytesent << std::endl;
      std::cout << std::flush; 
    } 
    delay(250);
  } // restart loop
    
  }

