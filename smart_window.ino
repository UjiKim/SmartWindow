#include <Stepper.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//Global Variables
//-----------------------Arduino Chip Memory
unsigned long prev_time = 0;     // 이전 시간을 저장 하는 변수 [ms]
unsigned long current_time = 0;  // 현재 시간을 저장 하는 변수 [ms]
bool isWindowClosed = true;
bool isBlindClosed = false;
enum State{ready = 0, windowclose, windowopen, blindclose, blindopen} state;

//-----------------------DHT 11
int Indoor_humidity = 0;      // 센싱한 내부습도
int Indoor_temparature = 0;   // 센싱한 내부온도
int Outdoor_humidity = 0;     // 센싱한 내부습도
int Outdoor_temparature = 0;  // 센싱한 내부온도

//-----------------------CDS
int cds_value = 0;            //센싱한 조도

//-----------------------빗물 감지
int rain_value = 0;            //빗물 감지 센싱

//Constants
const int steps = 64;             //스텝 모터 최대 스탭 2048
const int sense_duration = 2000;  //센싱 주기 1000 [ms] = 1 [sec] : 2초
const int brightness = 200;       //기준밝기
const int raindrop = 700;         //평상시 : rain_value >= raindrop, 비 : rain_value < raindrop  

//PIN Numbers
//-----------------------Outdoor
const int Outdoor_Raindrop = A0;  //빗방울 센서
const int Outdoor_CDS = A1;       //조도센서
const int Outdoor_DHT = A2;       //온습도센서
const int Indoor_DHT = A3;

//Class
Stepper stepper_Blind(steps, 8, 10, 9, 11);
Stepper stepper_Window(steps, 4, 6, 5, 7);
DHT dht_Outdoor(Outdoor_DHT, DHT22);
DHT dht_Indoor(Indoor_DHT, DHT22);
LiquidCrystal_I2C lcd(0x3f, 16, 2);    // LCD주소: 0x27 또는 0x3F
//functions

void rotate(Stepper stepper, int times, int num_step){
  for(int i = 0; i < 48*times; i++)
    stepper.step(num_step);
}

void setup() {
  prev_time = millis();

  state = ready;

  Serial.begin(9600);  //Serial 통신 115200 baud
  pinMode(Outdoor_CDS, INPUT);  //조도센서 인풋설정
  pinMode(Outdoor_Raindrop, INPUT);  //빗물감지센서 인풋설정
  
  dht_Outdoor.begin();  //dht 센서모듈
  dht_Indoor.begin();

  stepper_Blind.setSpeed(400);  //스탭모터 회전속도 설정
  stepper_Window.setSpeed(400);
  stepper_Blind.setSpeed(400);

  lcd.init();                      // LCD 초기화
  lcd.backlight();                // 백라이트 켜기
  lcd.clear();                    // LCD 화면을 지우고 (0, 0)의 위치에 커서를 놓습니다.
}

void loop() {
  current_time = millis();  //현재 시간

  switch(state){
    case ready :
      if (current_time - prev_time >= sense_duration) {
        //-----------------------Sensing
        Outdoor_humidity = dht_Outdoor.readHumidity();
        Outdoor_temparature = dht_Outdoor.readTemperature();
        Indoor_humidity = dht_Indoor.readHumidity();
        Indoor_temparature = dht_Indoor.readTemperature();
        cds_value = analogRead(Outdoor_CDS);
        rain_value = analogRead(Outdoor_Raindrop);
        
        //-----------------------Monitor
        Serial.print("Brightness : ");
        Serial.print(cds_value);
        Serial.print("  Rain : ");
        Serial.println(rain_value);
        Serial.print("Indoor Temp. : ");
        Serial.print(Indoor_temparature);
        Serial.print("  Indoor Humidity : ");
        Serial.println(Indoor_humidity);
        Serial.print("Outdoor Temp. : ");
        Serial.print(Outdoor_temparature);
        Serial.print("  Outdoor Humidity : ");
        Serial.println(Outdoor_humidity);
        Serial.print("isWindowClosed : ");
        Serial.print(isWindowClosed);
        Serial.print("  isBlindClosed : ");
        Serial.println(isBlindClosed);
        Serial.print("================================\n");

        lcd.clear(); // LCD 화면을 지우고 (0, 0)의 위치에 커서를 놓습니다.

        lcd.setCursor(0, 0);          // 첫번째 줄 문자열 출력
        lcd.print("T : ");            //온도
        lcd.print(Indoor_temparature);
        
        lcd.print(" H : ");          //습도
        lcd.print(Indoor_humidity);
        
        lcd.setCursor(0, 1);         // 두번째 줄 문자열 출력
        lcd.print("B : ");           //조도 
        lcd.print(cds_value);

        //-----------------------State
        if(isWindowClosed && !isBlindClosed){
          if((Outdoor_humidity < Indoor_humidity)&&(rain_value >= raindrop))
            state = windowopen;
        }
        if(!isWindowClosed && !isBlindClosed){
          if((Outdoor_humidity > Indoor_humidity)&&(rain_value >= raindrop))
            state = windowclose;
          if((rain_value < raindrop))
            state = windowclose;
        }
        if(isWindowClosed && !isBlindClosed && (cds_value <= brightness)) state = blindclose;
        if(isWindowClosed && isBlindClosed && (cds_value > brightness)) state = blindopen;
        if(!isWindowClosed && (Outdoor_temparature > Indoor_temparature) && (Indoor_temparature > 28)) state = windowclose;
        if(isWindowClsoed && Outdoor_humidity < Indoor_humidity) state = windowopen;

        prev_time = current_time;  // time update  
      }
      break;
    case windowopen :
      Serial.print("Window is opening...\n");

      lcd.clear(); // LCD 화면을 지우고 (0, 0)의 위치에 커서를 놓습니다.
      lcd.setCursor(0, 0);         // 첫번째 줄 문자열 출력
      lcd.print("open Window");

      rotate(stepper_Window, 1, steps);

      isWindowClosed = false;
      state = ready;
      break;
    case windowclose :
      Serial.print("Window is closing\n");

      lcd.clear(); // LCD 화면을 지우고 (0, 0)의 위치에 커서를 놓습니다.
      lcd.setCursor(0, 0);         // 첫번째 줄 문자열 출력
      lcd.print("close Window");

      rotate(stepper_Window, 1, -steps);

      isWindowClosed = true;
      state = ready;
      break;
    case blindopen :
      Serial.print("Blind is opening...\n");

      lcd.clear(); // LCD 화면을 지우고 (0, 0)의 위치에 커서를 놓습니다.
      lcd.setCursor(0, 0);         // 첫번째 줄 문자열 출력
      lcd.print("open Blind");

      rotate(stepper_Blind,1,steps);

      isBlindClosed = false;
      state = ready;
      break; 
    case blindclose :
      Serial.print("Blind is closing...\n");

      lcd.clear(); // LCD 화면을 지우고 (0, 0)의 위치에 커서를 놓습니다.
      lcd.setCursor(0, 0);         // 첫번째 줄 문자열 출력
      lcd.print("close Blind");
      
      rotate(stepper_Blind,1,-steps);

      isBlindClosed = true;
      state = ready;
      break;
  }
}
