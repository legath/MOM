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
const int DATA_SIZE = 10;        // 10byte data (2byte version + 8byte tag)
const int DATA_VERSION_SIZE = 2; // 2byte version (actual meaning of these two bytes may vary)
const int DATA_TAG_SIZE = 8;     // 8byte tag
const int CHECKSUM_SIZE = 2;     // 2byte checksum
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

uint32_t one_shot_was1=0;
uint32_t one_shot_was2 =0;

bool check_oneshot1=false;
bool check_oneshot2=false;

// Callback in case of an error
void errorCallback(cmd_error* e) {
    CommandError cmdError(e); // Create wrapper object

    //Serial.print("ERROR: ");
    //Serial.println(cmdError.toString());

}


void weightCallback(cmd* cmdPtr) {
    Command cmd(cmdPtr);
    double weight = scale.get_value(10);
    Serial.print("WEIGHT:");
    Serial.println(weight);
   
}

void lock_TOP_Callback(cmd* cmdPtr) {
    Command cmd(cmdPtr);
    Argument strArg    = cmd.getArgument("str");
    String strValue = strArg.getValue();
    if (strValue == "up")
    {
      digitalWrite(lockPinTop, HIGH);
      check_oneshot1=true;
      one_shot_was1=millis();
      //Serial.println("top lock up");

    }
    else{
      //Serial.println("top nothing to do");
    }
    }

void lock_BOTTOM_Callback(cmd* cmdPtr) {
    Command cmd(cmdPtr);
    Argument strArg    = cmd.getArgument("str");
    String strValue = strArg.getValue();
    if (strValue == "up")
    {
      digitalWrite(lockPinBottom, HIGH);
      check_oneshot2 = true;
      one_shot_was2 = millis();
      //Serial.println("bot lock up");

    }
    else{
      //Serial.println("bot nothing to do");
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
  if(check_oneshot1){
    if(millis()>= one_shot_was1 + 5000) //1 секунда
    {
      check_oneshot1 = false;
      digitalWrite(lockPinTop, LOW);
    }
  }
  if(check_oneshot2){
    if(millis()>= one_shot_was2 + 10000) //10 секунд
    {
      check_oneshot2 = false;
      digitalWrite(lockPinBottom, LOW);
    }
  }

  if (Serial.available()) {
        // Read out string from the serial monitor
        String input = Serial.readStringUntil('\n');

        // Echo the user input
        //Serial.print("# ");
        //Serial.println(input);

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
        Serial.print("FOUND_TAG:");
        /*for (int i = 1 ; i< BUFFER_SIZE; i++){
          Serial.print((char)buffer[i]);
        }
        Serial.print('\n');*/
        extract_tag();
        Serial.print('\n');
      } else { // something is wrong... start again looking for preamble (value: 2)
        buffer_index = 0;
        return;
      }
    }    
  }    
}


void extract_tag() {
    uint8_t msg_head = buffer[0];
    uint8_t *msg_data = buffer + 1; // 10 byte => data contains 2byte version + 8byte tag
    uint8_t *msg_data_version = msg_data;
    uint8_t *msg_data_tag = msg_data + 2;
    uint8_t *msg_checksum = buffer + 11; // 2 byte
    uint8_t msg_tail = buffer[13];

    // print message that was sent from RDM630/RDM6300
    //Serial.println("--------");

    ///Serial.print("Message-Head: ");
    //Serial.println(msg_head);

    //Serial.println("Message-Data (HEX): ");
    for (int i = 0; i < DATA_VERSION_SIZE; ++i) {
        Serial.print(char(msg_data_version[i]));
    }
    /*Serial.println(" (version)");
    for (int i = 0; i < DATA_TAG_SIZE; ++i) {
        Serial.print(char(msg_data_tag[i]));
    }
    Serial.println(" (tag)");

    Serial.print("Message-Checksum (HEX): ");
    for (int i = 0; i < CHECKSUM_SIZE; ++i) {
        Serial.print(char(msg_checksum[i]));
    }
    Serial.println("");

    Serial.print("Message-Tail: ");
    Serial.println(msg_tail);

    Serial.println("--");

    long tag = hexstr_to_value(msg_data_tag, DATA_TAG_SIZE);
    Serial.print("Extracted Tag: ");
    Serial.println(tag);

    long checksum = 0;
    for (int i = 0; i < DATA_SIZE; i += CHECKSUM_SIZE) {
        long val = hexstr_to_value(msg_data + i, CHECKSUM_SIZE);
        checksum ^= val;
    }
    Serial.print("Extracted Checksum (HEX): ");
    Serial.print(checksum, HEX);
    if (checksum == hexstr_to_value(msg_checksum, CHECKSUM_SIZE)) { // compare calculated checksum to retrieved checksum
        Serial.print(" (OK)"); // calculated checksum corresponds to transmitted checksum!
    } else {
        Serial.print(" (NOT OK)"); // checksums do not match
    }

    Serial.println("");
    Serial.println("--------");

    return tag;*/
}

long hexstr_to_value(char *str, unsigned int length) { // converts a hexadecimal value (encoded as ASCII string) to a numeric value
    char* copy = malloc((sizeof(char) * length) + 1);
    memcpy(copy, str, sizeof(char) * length);
    copy[length] = '\0';
    // the variable "copy" is a copy of the parameter "str". "copy" has an additional '\0' element to make sure that "str" is null-terminated.
    long value = strtol(copy, NULL, 16);  // strtol converts a null-terminated string to a long value
    free(copy); // clean up
    return value;
}