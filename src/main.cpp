#include <SoftwareSerial.h>
#include <HX711.h>

#include <SeeedRFID.h>
#define sensetivity 100 //чувствительность датчиков веса

#define lockPinTop 9//пин подключения реле верхнего замка
#define lockPinBottom 2//пин подключения реле нижнего замка
#define Reader_RX  8// пин подключения TX RFID
#define Reader_TX  7// пин подключения RX RFID
#define LOADCELL_DOUT_PIN A0//пин подключения DO АЦП
#define LOADCELL_SCK_PIN A1//пин подключения SCK АЦП


#define STRIP_PIN 4     // пин светодиода
#define NUMLEDS 1      // кол-во светодиодов
#include <microLED.h>
byte newCard[] = {};
byte oldCard[] = {1};
byte count = 0; //счетчик предметов
char rec; //байт для приема данных

long weight0 = 0; // переменная для хранения предыдущего измерения веса
long weight1 = 0; // переменная для хранения предыдущего измерения веса
long weight2 = 0; // переменная для хранения предыдущего измерения веса
bool cou = 0; //флаг счетчика инструмента
unsigned long weightCheck = 0;
unsigned long workTime = 0 ;


//SoftwareSerial RFID(RX, TX);// последовательный порт для RFID
HX711 scale; //подключаем датчики веса
microLED<NUMLEDS, STRIP_PIN, MLED_NO_CLOCK, LED_WS2812, ORDER_GRB> led; // инициализимруем светодиод

SoftwareSerial RFID(Reader_RX, Reader_TX);

unsigned char buffer[64];   
//int count = 0;

void setup() {
  Serial.begin(9600);
  RFID.begin(9600);
  pinMode(lockPinTop, OUTPUT);
  pinMode(lockPinBottom, OUTPUT);
  
  
  led.setBrightness(255);//устанавливаем яркость светодиода

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN); //подключаем АЦП к пинам
  scale.tare();                // устанавливаем значение веса в 0
  Serial.println("Ready");
  led.clear();
  led.show(); // вывод изменений на светодиод
}
void clearBufferArray();
void loop() {
 
    // if date is coming from software serial port ==> data is coming from SoftSerial shield
	    if (RFID.available())              
	    {
	        while(RFID.available())               // reading data into char array
	        {
	            buffer[count++] = RFID.read();      // writing data into array
	            if(count == 64)break;
	        }
	        Serial.write(buffer, count);     // if no data transmission ends, write buffer to hardware serial port
	        clearBufferArray();             // call clearBufferArray function to clear the stored data from the array
	        count = 0;                      // set counter of while loop to zero
	    }
	  
	}
	void clearBufferArray()                
	{
	    // clear all index of array with command NULL
	    for (int i=0; i<count; i++)
	    {
	        buffer[i]=NULL;
	    }  


 
}
