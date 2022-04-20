#include <SoftwareSerial.h>
#include <HX711.h>
#include <microLED.h>
#include <SimpleCLI.h>

//FOUND_TAG: 3A00DD8D6208
//FOUND_TAG: 3A00DD8D6208
//FOUND_TAG: 3A00DD8D6208
//FOUND_TAG: 3A00DD8D6208
//FOUND_TAG: 3A00DEDD340D


#define lockPinTop 9//пин подключения реле верхнего замка
#define lockPinBottom 2//пин подключения реле нижнего замка
#define Reader_RX  8// пин подключения TX RFID
#define Reader_TX  7// пин подключения RX RFID
#define LOADCELL_DOUT_PIN A0//пин подключения DO АЦП
#define LOADCELL_SCK_PIN A1//пин подключения SCK АЦП
#define STRIP_PIN 4     // пин светодиода
#define NUMLEDS 1      // кол-во светодиодов


const int BUFFER_SIZE = 14; // RFID DATA FRAME FORMAT: 1byte head (value: 2), 10byte data (2byte version + 8byte tag), 2byte checksum, 1byte tail (value: 3)

byte count = 0; //счетчик предметов
bool cou = 0; //флаг счетчика инструмента
unsigned long weightCheck = 0;
unsigned long workTime = 0 ;

HX711 scale; //подключаем датчики веса
microLED<NUMLEDS, STRIP_PIN, MLED_NO_CLOCK, LED_WS2812, ORDER_GRB> led; // инициализимруем светодиод
SoftwareSerial ssrfid(Reader_RX, Reader_TX);

SimpleCLI cli;

Command cmdLed;
Command cmdLockTop;
Command cmdLockBot;
Command cmdWeight;

uint8_t buffer[BUFFER_SIZE]; // used to store an incoming data frame 
int buffer_index = 0;


// Callback in case of an error
void errorCallback(cmd_error* e) {
    CommandError cmdError(e); // Create wrapper object

    Serial.print("ERROR: ");
    Serial.println(cmdError.toString());

}


void weightCallback(cmd* cmdPtr) {
    Command cmd(cmdPtr);
    double weight = scale.get_value(10);
    Serial.print("WEIGHT: ");
    Serial.println(weight);
   
}

void lock_TOP_Callback(cmd* cmdPtr) {
    Command cmd(cmdPtr);
    Argument strArg    = cmd.getArgument("str");
    String strValue = strArg.getValue();
    if (strValue == "up")
    {
      digitalWrite(lockPinTop, HIGH);
      Serial.println("top lock up");

    }
    else if (strValue == "down"){
      digitalWrite(lockPinTop, LOW);
      Serial.println("top lock down");
    
    }else{
      Serial.println("top nothing to do");
    }
    }

void lock_BOTTOM_Callback(cmd* cmdPtr) {
    Command cmd(cmdPtr);
    Argument strArg    = cmd.getArgument("str");
    String strValue = strArg.getValue();
    if (strValue == "up")
    {
      digitalWrite(lockPinBottom, HIGH);
      Serial.println("bot lock up");

    }
    else if (strValue == "down"){
      digitalWrite(lockPinBottom, LOW);
      Serial.println("bot lock down");
    
    }else{
      Serial.println("bot nothing to do");
    }

    
}

// Callback function for ping command
void ledCallback(cmd* c) {
    Command cmd(c); // Create wrapper object

    // Get arguments
    
    Argument strArg    = cmd.getArgument("str");
    String strValue = strArg.getValue();
    if (strValue == "red")
    {
      led.set(0, mRGB(255, 0, 0));
      led.show(); 
    }
    else if (strValue == "green"){
      led.set(0, mRGB(0, 255, 0));
      led.show(); 
    }
    else if (strValue == "blue"){
      led.set(0, mRGB(0, 0, 255));
      led.show(); 
    }else{
      led.clear();
      led.show();
    }
    Serial.println("set_color: done");
    
}

void setup() {
  Serial.begin(9600);
  ssrfid.begin(9600);
  pinMode(lockPinTop, OUTPUT);
  pinMode(lockPinBottom, OUTPUT);
  led.setBrightness(255);//устанавливаем яркость светодиода
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN); //подключаем АЦП к пинам
  scale.tare();                // устанавливаем значение веса в 0
  Serial.println("READY");
  led.clear();
  led.show(); // вывод изменений на светодиод
  Serial.flush();

  cli.setOnError(errorCallback);


  //cli
  cmdLed = cli.addCommand("set_color", ledCallback);
  cmdLed.addPositionalArgument("str", "none");

  cmdLockTop = cli.addCommand("lock_top", lock_TOP_Callback);
  cmdLockTop.addPositionalArgument("str", "none");

  cmdLockBot = cli.addCommand("lock_bot", lock_BOTTOM_Callback);
  cmdLockBot.addPositionalArgument("str", "none");

  cmdWeight= cli.addCommand("weight", weightCallback);

}


void loop() {
  if (Serial.available()) {
        // Read out string from the serial monitor
        String input = Serial.readStringUntil('\n');

        // Echo the user input
        Serial.print("# ");
        Serial.println(input);

        // Parse the user input into the CLI
        cli.parse(input);
  }
  if (ssrfid.available() > 0){
    bool print_tag = false;
    
    int ssvalue = ssrfid.read(); // read 
    if (ssvalue == -1) { // no data was read
      return;
    }

    if (ssvalue == 2) { // RDM630/RDM6300 found a tag => tag incoming 
      buffer_index = 0;
    } else if (ssvalue == 3) { // tag has been fully transmitted       
      print_tag = true; // extract tag at the end of the function call
    }

    if (buffer_index >= BUFFER_SIZE) { // checking for a buffer overflow (It's very unlikely that an buffer overflow comes up!)
      //Serial.println("Error: Buffer overflow detected!");
      return;
    }
    
    buffer[buffer_index++] = ssvalue; // everything is alright => copy current value to buffer

    if (print_tag == true) {
      if (buffer_index == BUFFER_SIZE) {
        Serial.print("FOUND_TAG: ");
        Serial.println((char*)(buffer));
        //unsigned tag = extract_tag();
      } else { // something is wrong... start again looking for preamble (value: 2)
        buffer_index = 0;
        return;
      }
    }    
  }    
}
