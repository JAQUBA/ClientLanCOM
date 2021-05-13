#include <SPI.h>
#include <UIPEthernet.h>
#include <avr/wdt.h>

#define STATIC

byte MACADDRESS[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

#define UARTBAUD 9600

#ifdef STATIC
  #define MYIPADDR IPAddress(192,168,0,101)
  #define MYIPMASK IPAddress(255,255,255,0)
  #define MYDNS IPAddress(192,168,0,1)
  #define MYGW IPAddress(192,168,0,1)
#endif

#define LED 8
#define SERIAL_TIMEOUT 5

IPAddress server(192,168,0,102);
#define PORT 5556

EthernetClient client;

void setup() {
  Serial.begin(UARTBAUD);
  
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  delay(200);
  digitalWrite(4, HIGH);
  delay(2000);
  
#ifdef STATIC
  Ethernet.begin(MACADDRESS, MYIPADDR, MYDNS, MYGW, MYIPMASK);
#else
  Ethernet.begin(MACADDRESS);
#endif
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  wdt_enable(WDTO_8S);
}

void connect() {
  client.stop();
  if (!client.connect(server, PORT)) {
    delay(1000);
  }
}

unsigned long previousPing = 0;
unsigned long interruptionPing = 4000;

unsigned long previousLED = 0;
unsigned long interruptionLED = 500;
bool statusLED = false;

unsigned long previousSerial = 0;

char serialBuffer[256];
unsigned int serialBufferNum = 0;

void loop() {
  wdt_reset();
  Ethernet.maintain();
  unsigned long currentMillis = millis();


  if(previousLED+interruptionLED < currentMillis) {
    previousLED = currentMillis;
    digitalWrite(LED, statusLED=!statusLED);
  }
  
  if(previousPing+interruptionPing < currentMillis) {
    previousPing = currentMillis;
    client.write(0xFF);
  }
  
  if (!client.connected()) {
    connect();
    return;
  }
  
  if(Serial.available() > 0) {
    previousSerial = currentMillis;

    while(Serial.available() > 0) {
      char recv = (char)Serial.read();
      serialBuffer[serialBufferNum] = recv;
      serialBufferNum++;
    }
  }

  if((previousSerial+SERIAL_TIMEOUT < currentMillis) && serialBufferNum > 0) {
    client.flush();
    client.write(serialBuffer, serialBufferNum);
    serialBufferNum = 0;
  }

  while(client.available() > 0) {
    char recv = (char)client.read();
    Serial.print(recv);
    previousPing = currentMillis;
  }
}
