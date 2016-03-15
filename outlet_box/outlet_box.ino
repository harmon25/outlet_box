#include <avr/pgmspace.h>
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include "printf.h"

const int greenB = 4;  // the pin numbelsr of the pushbutton
const int greenL = 7;  // the pin number of the LED

const int relayG = 3;  // the pin number of the green relay 
const int relayR = 2;  // the number of the Red relay

const int redB = 5;    // the pin number of the red pushbutton
const int redL = 6;    // the pin number of the red LED

const uint16_t this_node =  01; 
uint16_t to = 00;   
const unsigned long interval = 50000; // ms       // Delay manager to send pings regularly.
unsigned long last_time_sent;


// Variables will change:
int GledState = LOW;         // the current state of the output pin
int GbuttonState;             // the current reading from the input pin
int GlastButtonState = LOW;   // the previous reading from the input pin

// Variables will change:
int RledState = LOW;         // the current state of the output pin
int RbuttonState;             // the current reading from the input pin
int RlastButtonState = LOW;   // the previous reading from the input pin

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long RlastDebounceTime = 0;  // the last time the output pin was toggled
long GlastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 80;    // the debounce time; increase if the output flickers

bool send_S(uint16_t to);
void handle_R(RF24NetworkHeader& header);
void handle_G(RF24NetworkHeader& header);
void handle_SW(RF24NetworkHeader& header);
void handle_S(RF24NetworkHeader& header);
void send_state(uint16_t to);
void handle_G(RF24NetworkHeader& header);

// state message
struct S_message_t
{
  bool red;    //state of red outlet 0/1
  bool green;  //state of green outlet 0/1
};

S_message_t outlet_state = {0,0};

// state message
struct T_message_t
{
  char message[20];    // a message
};



//setup radio on 9/10
RF24 radio(9,10);                             
RF24Network network(radio); 

void setup() {
  Serial.begin(115200);
  printf_begin();
  printf_P(PSTR("\n\rOUTLETBOX v1.3!!!!\n\r"));
  
  pinMode(greenB, INPUT);
  pinMode(greenL, OUTPUT);
  pinMode(redB, INPUT);
  pinMode(redL, OUTPUT);

  pinMode(relayG, OUTPUT);
  pinMode(relayR, OUTPUT);

  // set initial LED state
  digitalWrite(greenL, outlet_state.green);
  digitalWrite(redL, outlet_state.red);

  // set initial RELAY state, oposite of LED
  digitalWrite(relayG, !outlet_state.green);
  digitalWrite(relayR, !outlet_state.red);
 
  SPI.begin();           // Bring up the RF network
  radio.begin();
  radio.setPALevel(RF24_PA_HIGH);
  //radio.setDataRate(RF24_250KBPS);
  network.begin(/*channel*/ 115, /*node address*/ this_node );
  
  // send state right away to tell base you are alive!
  send_state(to);
}

void loop() {
  // read the state of the switch into a local variable:
  int readingR = digitalRead(redB);
  int readingG = digitalRead(greenB);
 
  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH),  and you've waited
  // long enough since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (readingR !=  RbuttonState) {
    // reset red debouncing timer
    RlastDebounceTime = millis();
  } else if(readingG != GbuttonState ) {
    // reset green debouncing timer
    GlastDebounceTime = millis();
  }
  
  if ((millis() - GlastDebounceTime) > debounceDelay || (millis() -  RlastDebounceTime) > debounceDelay ) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:
    // if the button state has changed:
    if (readingG != GbuttonState) {
      GbuttonState = readingG;
      // only toggle the LED if the new button state is HIGH
      if (GbuttonState == HIGH) {
        // toggle the Green LED & Relay from button press
        outlet_state.green = !outlet_state.green;
        digitalWrite(greenL, outlet_state.green);
        digitalWrite(relayG, !outlet_state.green);  // should be the 'same' as LED, in the case of relays ON is LOW, OFF is HIGH
        send_state(to);
       }
    }

    if( readingR != RbuttonState){
      RbuttonState = readingR;
      if (RbuttonState == HIGH) {
        // toggle the Green LED & Relay from button press :
        outlet_state.red = !outlet_state.red;
        digitalWrite(redL, outlet_state.red);
        digitalWrite(relayR, !outlet_state.red);
        send_state(to);
      }
    }
  }
  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  RlastButtonState = readingR;
  GlastButtonState = readingG;

   // check if we are getting a message  
   network.update();                                      // Pump the network regularly

   while ( network.available() )  {                      // Is there anything ready for us?   
    RF24NetworkHeader header;                            // If so, take a look at it
    network.peek(header);
      switch (header.type){                              // Dispatch the message to the correct handler.
        case 'R': handle_SW(header); break;
        case 'G': handle_SW(header); break;
        case 'S': handle_S(header); break;
        default:  printf_P(PSTR("*** WARNING *** Unknown message type %c\n\r"),header.type);
                  network.read(header,0,0);
                  break;
      };
    }
  
 // unsigned long now = millis();                    // Send a status ping to parent node every 'interval' ms
 // if ( now - last_time_sent >= interval ){
 //   last_time_sent = now;
 //  }
}

void send_state(uint16_t to)
{
   bool ok;
  ok = send_S(to);
    if (ok){           // Notify us of the result
        printf_P(PSTR("Sent state OK \n green: %i \n red: %i \n\r"), outlet_state.green, outlet_state.red);
    } else{
       printf_P(PSTR("Send state FAILED \n green: %i \n red: %i \n\r"), outlet_state.green, outlet_state.red);
    }
}


/**
 * Send a 'S' message, the current state of outlets
 */
bool send_S(uint16_t to)
{
  RF24NetworkHeader header(/*to node*/ to, /*type*/ 'S' /*State*/);
  printf_P(PSTR("---------------------------------\n\r"));
  printf_P(PSTR("Sending state to base\n\r"));
  return network.write(header,&outlet_state,sizeof(outlet_state));
}

/**
 * Handle a 'R' message
 * Add the node to the list of active nodes
 */
void handle_S(RF24NetworkHeader& header){
  T_message_t payload;                                                                    // The 'T' message is just a ulong, containing the time
  network.read(header,&payload,sizeof(payload));
  printf_P(PSTR("Received message: %s from: 0%o\n\r"),payload.message,header.from_node);
  send_state(to);  //send state back so the base knows whats up
}

/**
 * Handle an 'G' message, the active node list
 */
void handle_SW(RF24NetworkHeader& header)
{
  T_message_t payload;
  network.read(header,&payload,sizeof(payload));
  printf_P(PSTR("Received message: %s from: 0%o\n\r"),payload.message,header.from_node);
 switch (header.type){
    case 'R': outlet_state.red = !outlet_state.red;
              //set the state of led and buttons
              digitalWrite(redL, outlet_state.red);
              digitalWrite(relayR, !outlet_state.red);
              break;
    case 'G':  outlet_state.green = !outlet_state.green;
               //set the state of led and buttons
               digitalWrite(greenL, outlet_state.green);
               digitalWrite(relayG, !outlet_state.green);
               break;
    default:  printf_P(PSTR("*** WARNING *** Unknown outlet type %c\n\r"),header.type);
              break;
       };
 
  send_state(to); //send state back so the base knows whats up
}


