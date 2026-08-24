#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define DHT_TYPE DHT11
#define DHT_PIN 4 
#define IR_PIN 34
#define Temp_high_bit (1<<0)
#define Object_detected_bit (1<<1)
#define Humidity_high_bit (1<<2)
#define GREEN_LED 25
#define RED_LED 13
#define BUZZER 32
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 
#define OLED_RESET -1
#define OLED_ADDR 0x3C


Adafruit_SSD1306 display(SCREEN_WIDTH,SCREEN_HEIGHT,&Wire,OLED_RESET);
DHT dht(DHT_PIN,DHT_TYPE);
QueueHandle_t sensorReading; 
QueueHandle_t analysisQueue;
EventGroupHandle_t systemEvents;
TaskHandle_t outputTaskHandle;
SemaphoreHandle_t serialMutex;
TimerHandle_t heartBeatTimer;

uint32_t upTimeCounter = 0;


struct Data{
  int IR_value;
  float temperature,humidity;
};

enum systemState{
  IDLE,
  Normal,
  Alert,
};

void sensorTask(void *parameter);
void controllerTask(void *parameter);
void analysisTask(void *parameter);
String printCurrentState(systemState currentState);
void setdevices();
void outputTask(void *parameter);
void OLED_checker();
void Readings_ON_OLED(float temperature, float humidity,int IR_value,systemState currentState);
void heartBeatCallBack(TimerHandle_t xTimer);

void setup(){
  Serial.begin(115200);
  dht.begin();
  OLED_checker();
  setdevices();
  pinMode(IR_PIN,INPUT);
  sensorReading = xQueueCreate(5,sizeof(Data));
  systemEvents = xEventGroupCreate();
  analysisQueue = xQueueCreate(5,sizeof(Data));
  serialMutex = xSemaphoreCreateMutex();
  heartBeatTimer = xTimerCreate("Heartbeat",pdMS_TO_TICKS(5000),pdTRUE,0,heartBeatCallBack);
  xTimerStart(heartBeatTimer,0);
  xTaskCreatePinnedToCore(sensorTask,"Sensor Task",5000,NULL,2,NULL,1);
  xTaskCreatePinnedToCore(controllerTask,"Controller Task",5000,NULL,2,NULL,1);
  xTaskCreatePinnedToCore(analysisTask,"Analysis Task",5000,NULL,3,NULL,1);
  xTaskCreatePinnedToCore(outputTask,"Output Task",5000,NULL,1,&outputTaskHandle,1);

}

void loop(){
  
}

void sensorTask(void *parameter){

  Data sensorData;
  while(true){
    sensorData.IR_value = digitalRead(IR_PIN);
    sensorData.temperature = dht.readTemperature();
    sensorData.humidity = dht.readHumidity();
    xQueueSend(sensorReading,&sensorData,portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void controllerTask(void *parameter){
  Data receivedData;
  while(true){
    xQueueReceive(sensorReading,&receivedData,portMAX_DELAY);
    xSemaphoreTake(serialMutex,portMAX_DELAY);
    upTimeCounter ++;
    xSemaphoreGive(serialMutex);

    Serial.print("IR: ");
    Serial.print(receivedData.IR_value);
    Serial.print("   ");
  
    if(isnan(receivedData.temperature) || isnan(receivedData.humidity)){
      Serial.print("Invalid DHT11 Reading");
    }
    else{
      Serial.print("Temperature: ");
      Serial.print(receivedData.temperature);
      Serial.print("   ");
      Serial.print("Humidity:  ");
      Serial.println(receivedData.humidity);
    }
    Serial.print("UpTime Counter: ");
    Serial.print(upTimeCounter);
    Serial.println(" s");
  
    xQueueSend(analysisQueue,&receivedData,portMAX_DELAY);

  }
}

void analysisTask(void *parameter){
  Data analyticalData;
  systemState currentState = IDLE;
  EventBits_t events;
  while(true){
    xQueueReceive(analysisQueue,&analyticalData,portMAX_DELAY);


    if(isnan(analyticalData.temperature) || isnan(analyticalData.humidity)){
      currentState = IDLE;
    }
    else{

      if(analyticalData.temperature > 35){
        xEventGroupSetBits(systemEvents, Temp_high_bit);
      }
      else{
        xEventGroupClearBits(systemEvents, Temp_high_bit);
      }

      if(analyticalData.humidity > 75){
        xEventGroupSetBits(systemEvents,Humidity_high_bit);
      }
      else{
       xEventGroupClearBits(systemEvents,Humidity_high_bit);
      }

      if(analyticalData.IR_value == 0){
        xEventGroupSetBits(systemEvents,Object_detected_bit);
      }
      else{
        xEventGroupClearBits(systemEvents,Object_detected_bit);
      }
      events = xEventGroupGetBits(systemEvents);

      if(events & (Temp_high_bit | Humidity_high_bit | Object_detected_bit)){
        currentState = Alert;
      }
      else{
        currentState = Normal;
      }
    }
    xSemaphoreTake(serialMutex,portMAX_DELAY);
    upTimeCounter ++;
    xSemaphoreGive(serialMutex);
    xTaskNotify(outputTaskHandle,currentState,eSetValueWithOverwrite);
    Readings_ON_OLED(analyticalData.temperature,analyticalData.humidity,analyticalData.IR_value,currentState);
    Serial.print("SYSTEM: ");
    Serial.println(printCurrentState(currentState));

  }
}

String printCurrentState(systemState currentState){
  if(currentState == Normal){
    return "NORMAL";
  }
  else if(currentState == Alert){
    return "ALERT";
  }
  else{
    return "IDLE";
  }
}

void setdevices(){
  pinMode(GREEN_LED,OUTPUT);
  pinMode(RED_LED,OUTPUT);
  pinMode(BUZZER,OUTPUT);
  digitalWrite(GREEN_LED,LOW);
  digitalWrite(RED_LED,LOW);
  digitalWrite(BUZZER,LOW);
}

void outputTask(void *parameter){

  uint32_t notification;
  while(true){
    xTaskNotifyWait(0, 0, &notification, portMAX_DELAY);

    if(notification == Normal){
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, LOW);
      digitalWrite(BUZZER, LOW);
    }
    else if(notification == Alert){
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, HIGH);
      digitalWrite(BUZZER, HIGH);
    }
    else{ 
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, LOW);
      digitalWrite(BUZZER, LOW);
    }
  }
}

void OLED_checker(){
  if(!display.begin(SSD1306_SWITCHCAPVCC,OLED_ADDR)){
    Serial.println("OLED Not Found!!");
  }
}

void Readings_ON_OLED(float temperature, float humidity,int IR_value,systemState currentState){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0,0);
  display.print("IR: ");
  display.println(IR_value);

  display.setCursor(0,12);
  display.print("Humidity: ");
  display.println(humidity);

  display.setCursor(0,24);
  display.print("Temp: ");
  display.println(temperature);

  display.setCursor(0,36);
  display.print("State: ");
  display.println(printCurrentState(currentState));

  display.setCursor(0,48);
  display.print("Up count: ");
  display.println(upTimeCounter);

  display.display();
}

void heartBeatCallBack(TimerHandle_t xTimer){
  xSemaphoreTake(serialMutex,portMAX_DELAY);
  Serial.print("[Heartbeat]Uptime: ");
  Serial.print(upTimeCounter);
  Serial.println("s");
  xSemaphoreGive(serialMutex);

}



