#include <SoftwareSerial.h>
#include <HX711.h>

#define sensetivity 100 //чувствительность датчиков веса

#define lockPinTop 9//пин подключения реле верхнего замка
#define lockPinBottom 2//пин подключения реле нижнего замка
#define RX  8// пин подключения TX RFID
#define TX  7// пин подключения RX RFID
#define LOADCELL_DOUT_PIN A0//пин подключения DO АЦП
#define LOADCELL_SCK_PIN A1//пин подключения SCK АЦП
#define doplerPin A3 //пин подключения доплеровского радара

#define STRIP_PIN 4     // пин светодиода
#define NUMLEDS 1      // кол-во светодиодов
#include <microLED.h>
byte newCard[] = {};
byte oldCard[] = {1};
byte count = 0; //счетчик предметов
char rec; //байт для приема данных
bool dopler = 0; //состояние радара
long weight0 = 0; // переменная для хранения предыдущего измерения веса
long weight1 = 0; // переменная для хранения предыдущего измерения веса
long weight2 = 0; // переменная для хранения предыдущего измерения веса
bool cou = 0; //флаг счетчика инструмента
unsigned long weightCheck = 0;
unsigned long workTime = 0 ;


SoftwareSerial RFID(RX, TX);// последовательный порт для RFID
HX711 scale; //подключаем датчики веса
microLED<NUMLEDS, STRIP_PIN, MLED_NO_CLOCK, LED_WS2812, ORDER_GRB> led; // инициализимруем светодиод

void readCard();


void setup() {
  Serial.begin(9600);
  RFID.begin(9600);
  pinMode(lockPinTop, OUTPUT);
  pinMode(lockPinBottom, OUTPUT);
  
  pinMode(doplerPin, INPUT);

  digitalWrite(lockPinBottom, HIGH);
  digitalWrite(lockPinTop, HIGH);
  delay(2000);
  digitalWrite(lockPinBottom, LOW);
  digitalWrite(lockPinTop, LOW);
  delay(2000);
  
  led.setBrightness(255);//устанавливаем яркость светодиода

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN); //подключаем АЦП к пинам
  scale.tare();                // устанавливаем значение веса в 0
  Serial.println("Ready");
  led.clear();
  led.show(); // вывод изменений на светодиод
}

void loop() {
  readCard();
  if (rec == 'N') {
    led.set(0, mRGB(255, 0, 0));// ставим красный
    rec = 0;
    led.show(); // вывод изменений на светодиод
    delay(1000);
    led.clear();
    led.show(); // вывод изменений на светодиод
  }
  if (rec == 'M') {
    Serial.println(count);
    digitalWrite(lockPinBottom, HIGH);
    led.set(0, mRGB(0, 255, 0));
    led.show(); // вывод изменений на светодиод
    delay(10000);
    digitalWrite(lockPinBottom, LOW);
    rec = 0;
    while (rec == 0) {
      led.set(0, mRGB(0, 255, 0));
      led.show(); // вывод изменений на светодиод
      delay(500);
      led.clear();
      led.show(); // вывод изменений на светодиод
      delay(500);
      readCard();
    }
    rec = 0;
    count = 0;
    Serial.println(count);
  }
  if (rec == 'W') {
    Serial.println(count);
    digitalWrite(lockPinTop, HIGH);
    led.set(0, mRGB(0, 255, 0));
    led.show(); // вывод изменений на светодиод
    delay(500);
    led.set(0, mRGB(0, 0, 255));
    led.show(); // вывод изменений на светодиод
    delay(1500);
    digitalWrite(lockPinTop, LOW);
    delay(1500);
    rec = 0;
    workTime = millis();
    while (rec == 0) {
      /*

        weight = scale.get_value(15);
        Serial.println(weight);
        dopler = digitalRead(doplerPin);
        Serial.println(dopler);
        if (scale.get_value(15) - weight > sensetivity && dopler == 1) {
        count++;
        Serial.println("detected");
        Serial.println(count);
        led.set(0, mRGB(0, 0, 255));
        led.show(); // вывод изменений на светодиод
        delay(1000);
        led.set(0, mRGB(0, 255, 0));
        led.show(); // вывод изменений на светодиод
      */
      led.set(0, mRGB(0, 255, 0));
      led.show(); // вывод изменений на светодиод
      dopler = digitalRead(doplerPin);

      if (millis() - weightCheck > 3000) {
        weight0 = scale.get_value(10);
        weightCheck = millis();
        //Serial.println(weight0);
      }
      // Serial.println(weight);
      while (dopler == 1) {
        dopler = digitalRead(doplerPin);
        led.set(0, mRGB(0, 0, 255)); //включаем желтый цвет
        led.show(); // вывод изменений на светодиод
        weight1 = scale.get_value(10);
        //Serial.print("Старый вес: ");
        //Serial.println(weight0);
        //Serial.print("Новый вес: ");
        //Serial.println(weight1);
        if (weight1 - weight0 > sensetivity && cou == 0) { //если показания весов увеличились более чем на размер чувствительности и предмет еще не был засчитан
          cou = 1;
          //Serial.println("1 detect");
        }
        // Serial.println(weight);
        // Serial.println("radar");
      }
      delay(1500);
      if (cou == 1) {
        //Serial.print("Старый вес: ");
        //Serial.println(weight0);
        //Serial.print("Новый вес: ");
        weight2 = scale.get_value(10);
        //Serial.println(weight2);
        if (weight2 - weight0 > sensetivity) { //если показания весов увеличились более чем на размер чувствительности и предмет еще не был засчитан
          count++;
          
          for (int i = 0 ; i < 14 ; i++)
          {
            Serial.write(oldCard[i]);


            //Serial.print(" ");
          }
          Serial.write(count);
        }
        weight0 = scale.get_value(10);
      }
      cou = 0;// сбрасываем флаг счетчика
      if (millis() - workTime > 600000) {
        rec = 'N';
      }
      readCard();
    }
  }
}

void readCard() {

  if (RFID.available() > 0)
  {
    delay(100);
    Serial.flush();
    //Serial.println("GO");
    for (int i = 0 ; i < 14 ; i++)
            {
              newCard[i]=RFID.read();
              Serial.write(newCard[i]);
              
             
            }
             
            Serial.write('O');
  }
  RFID.flush();
  for (int i = 0 ; i < 14 ; i++)
            {
              oldCard[i]=newCard[i];
              //Serial.write(oldCard[i]);
              
             
            }

    

    //Serial.println("Card detected");
  
  if (Serial.available() > 0)
  {
    rec = Serial.read();
  }
}
