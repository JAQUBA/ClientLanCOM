#define UIP_CONF_UDP 0
#define UIP_CONF_MAX_CONNECTIONS 1

#include <UIPEthernet.h>
#include <avr/wdt.h>

#define STATIC

#define ETH_RESET 4
byte MACADDRESS[] = {0xBE, 0xEF, 0x00, 0xBA, 0xBE, 0x00};
#ifdef STATIC
  #define MYIPADDR IPAddress(192,168,0,230)
  #define MYIPMASK IPAddress(255,255,255,0)
  #define MYDNS IPAddress(192,168,0,1)
  #define MYGW IPAddress(192,168,0,1)
#endif

IPAddress host(192,168,0,3);
#define PORT 5556

EthernetClient client;
EthernetServer server = EthernetServer(1000);

#define LED 7
unsigned long previousLED = 0;
unsigned long interruptionLED = 500;
bool statusLED = false;

#define SERIAL0_BAUD 9600
#define SERIAL0_TIMEOUT 6
#define SERIAL0_BUF_SIZE 255
unsigned long serial0Timestamp = 0;
char serial0Buffer[SERIAL0_BUF_SIZE];
unsigned int serial0BufferNum = 0;

#define SERIAL1_BAUD 19200
#define SERIAL1_TIMEOUT 6
#define SERIAL1_BUF_SIZE 255
unsigned long serial1Timestamp = 0;
char serial1Buffer[SERIAL1_BUF_SIZE];
unsigned int serial1BufferNum = 0;

void setup() {
  Serial.begin(SERIAL0_BAUD);
  Serial1.begin(SERIAL1_BAUD);
  
  pinMode(ETH_RESET, OUTPUT);
  digitalWrite(ETH_RESET, LOW);
  delay(200);
  digitalWrite(ETH_RESET, HIGH);
  delay(200);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, statusLED=!statusLED);
  
#ifdef STATIC
  Ethernet.begin(MACADDRESS, MYIPADDR, MYDNS, MYGW, MYIPMASK);
#else
  Ethernet.begin(MACADDRESS);
#endif

  server.begin();

  wdt_enable(WDTO_8S);
}

unsigned long previousPing = 0;
unsigned long interruptionPing = 4000;

void loop() {
  wdt_reset();
  Ethernet.maintain();
  unsigned long currentMillis = millis();

  if(previousLED+interruptionLED < currentMillis) {
    previousLED = currentMillis;
    digitalWrite(LED, statusLED=!statusLED);
  }
  size_t size;
  

  ////////////SERVICE
  EthernetClient sClient = server.available();
  if(sClient) {
    while((size = sClient.available()) > 0) {
      Serial1.print((char)sClient.read());
    }
  }
  while(Serial1.available() > 0) {
    serial1Timestamp = currentMillis;
    serial1Buffer[serial1BufferNum] = (char)Serial1.read();
    if(serial1BufferNum++ >= SERIAL1_BUF_SIZE-1) {
      serial1Timestamp = 0;
      break;
    }
  }
  if(serial1BufferNum > 0 && (serial1Timestamp+SERIAL1_TIMEOUT <= currentMillis)) {
    server.write(serial1Buffer, serial1BufferNum);
    serial1BufferNum = 0;
  }
  ///////////////////////////////////////////


  //////RADIO
  if(previousPing+interruptionPing < currentMillis) {
    previousPing = currentMillis;
    if(!client.connected() && !client.connect(host, PORT)) return;
    client.write(0xFF);
  }

  while((size = client.available()) > 0) {
    Serial.print((char)client.read());
    previousPing = currentMillis;
  }
  
  while(Serial.available() > 0) {
    serial0Timestamp = currentMillis;
    serial0Buffer[serial0BufferNum] = (char)Serial.read();
    if(serial0BufferNum++ >= SERIAL0_BUF_SIZE-1) {
      serial0Timestamp = 0;
      break;
    }
  }
  if(serial0BufferNum > 0 && (serial0Timestamp+SERIAL0_TIMEOUT < currentMillis)) {
    client.write(serial0Buffer, serial0BufferNum);
    serial0BufferNum = 0;
  }
}