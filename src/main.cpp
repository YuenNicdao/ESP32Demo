#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>


//Define the pins of the LED 
#define led1 0
#define led2 4
#define led3 15
#define trim 36
#define wifiLED 2

int led3_current_value; 

SemaphoreHandle_t Binary_Semaphore;
QueueHandle_t xQueue;

// WiFi Credentials
// const char *ssid = "Sebbyy";
// const char *password = "Ayokongasabi@123";
const char *ssid = "HondaTarp"; 
const char *password = "1234567890";  

// MQTT Credentials
const char *mqtt_broker = "broker.emqx.io";
const char *topic1 = "led3_status";
const char *topic2 = "led1_2_status";
const char *topic3 = "esp_status";

const char *mqtt_username = "yuenicdao";
const char *mqtt_password = "yuenicdao";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

//Tasks Instance and Handles
void wifimqttTask(void *pvParameters);  //WifiandMQTT Connection
void afbTask(void *pvParameters);      // analogfeedback
void ledTask(void *pvParameters);      //LED 1, 2, and 3 Task

TaskHandle_t wifimqttTask_handle = NULL; 
TaskHandle_t afbTask_handle = NULL; 
TaskHandle_t ledTask_handle = NULL; 


// Receive MQTT messages and store it in the queue
void callback(char *topic, byte *payload, unsigned int length) {
 Serial.print("Message arrived in topic: ");
 Serial.println(topic);
 Serial.print("Message:");
 String mqttMessage;

 for (int i = 0; i < length; i++)
 {
     Serial.print((char) payload[i]);
     mqttMessage += (char)payload[i];
 }

Serial.println();
Serial.println("-----------------------");
if(topic == "esp_status" && mqttMessage == "getStatus")
        {
            char led_1[1];
            char led_2[1];
            char led_3[3];
            itoa(EEPROM.read(0),led_1,10);
            itoa(EEPROM.read(1),led_2,10);
            itoa(led3_current_value,led_3,10);
            // Serial.println(led_1);
            // Serial.println(led_2);
            // Serial.println(led_3);
             client.publish(topic1, led_3);
             client.publish(topic2, led_1);
             client.publish(topic2, led_2);
        }
    else
    {
        xQueueSend (xQueue, &mqttMessage, portMAX_DELAY);
    }


}


void setup(){
    //Initialize the serial monitor
    Serial.begin(9600);
    //Set the pin modes of the attached hardware on the esp32
    //the internal pin of esp32 connected to the gpio pin 2 is used for the wifiLED
    pinMode(trim,INPUT);
    pinMode(led3,OUTPUT);
    pinMode(led1,OUTPUT);
    pinMode(led2,OUTPUT);
    pinMode(wifiLED,OUTPUT);
  
    digitalWrite(led3, LOW);

    //Intialize the EEPROM and set it to led
    EEPROM.begin(12);
    digitalWrite(led1, EEPROM.read(0));
    digitalWrite(led2, EEPROM.read(1));

    //Create a queue and semaphore
    xQueue = xQueueCreate(5,sizeof(char)*256);
    Binary_Semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(Binary_Semaphore);

    //Initialize the Tasks (wifimqtt, afb, & led)
    xTaskCreatePinnedToCore(wifimqttTask,"wifimqtt",10000,NULL,2,&wifimqttTask_handle,0);
    xTaskCreatePinnedToCore(afbTask,"led3_afb",2048,NULL,1,&afbTask_handle,1);
    xTaskCreatePinnedToCore(ledTask,"ledTask",2048,NULL,1,&afbTask_handle,1);   
}


void wifimqttTask(void * parameter){
    for(;;){
        if(WiFi.status() == WL_CONNECTED) // This will periodically check for the status of WiFi
        {
            if (!client.connected())
            {
                if (client.connect("ESP", mqtt_username, mqtt_password))
                    Serial.println("Connected to MQTT broker");
                    client.subscribe(topic1);
                    client.subscribe(topic2);
                  //  client.publish(topic3, "isConnected");
            }
            client.loop();
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        Serial.println("[WIFI] Connecting");

        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);

        digitalWrite(wifiLED,LOW); //Keep this turned off until the WiFi is connected to the network
        vTaskDelay(10000 / portTICK_PERIOD_MS); //Wait for 10seconds until the WiFi is connected

        if(WiFi.status() != WL_CONNECTED){
            // When we couldn't make a WiFi connection (or the timeout expired)
		    // Pause for 1 second and then retry.
            // Suspend the task for afb and led
            if(afbTask_handle != NULL) vTaskSuspend(afbTask_handle);
            if(ledTask_handle != NULL) vTaskSuspend(ledTask_handle);
            Serial.println("[WIFI] FAILED");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
			continue;
        }
        else
        {
        //Once the Wifi sucessfully connects, connect to the MQTT Broker and turn on the wifiStatus LED
        //Resume the suspended task
        Serial.println("[WIFI] Connected");

        if(afbTask_handle != NULL) vTaskResume(afbTask_handle);
        if(ledTask_handle != NULL) vTaskResume(ledTask_handle);

        client.setServer(mqtt_broker, mqtt_port);
        client.setCallback(callback);
        digitalWrite(wifiLED,HIGH);

        while (!client.connected())
        {
            String client_id = "esp32-client-";
            client_id += String(WiFi.macAddress());
            Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
            if (client.connect(client_id.c_str(), mqtt_username, mqtt_password))
            {
                Serial.println("Public EMQX-MQTT Broker connected");
                 client.subscribe(topic1);
                client.subscribe(topic2);
                client.subscribe(topic3);
               //client.publish(topic3, "isConnected");
            }
            else 
            {
                Serial.print("failed with state ");
                Serial.print(client.state());
                vTaskDelay(2000 / portTICK_PERIOD_MS);
            }
        }
        }

        
    }
}

void afbTask(void * parameter){

for(;;)
{
    //get the value from the analog feedback sensor and set it to the LED3 led pin
   int trimValue = map(analogRead(trim), 0, 4095,0,255);
    xSemaphoreTake(Binary_Semaphore, portMAX_DELAY);
    analogWrite(led3,trimValue); 
    xSemaphoreGive(Binary_Semaphore);
    vTaskDelay(200 / portTICK_PERIOD_MS); //declare an interval of 200 miliseconds to avoid overfeeding the task

    //If the value of the sensor does not change, do not send a message to the MQTT
    // a threshold value of +/- 1 is set to ensure that nothing will happen if for an instance, the analog feedback becomes unstable 
    if(led3_current_value != trimValue && led3_current_value != trimValue + 1 && led3_current_value != trimValue - 1 )
    {
    Serial.println(trimValue);
    char mapped_trimValue[3];
    ltoa(map(trimValue,0,255,0,100),mapped_trimValue,10);
    client.publish(topic1,mapped_trimValue);
    }
    led3_current_value = trimValue; //Save the analog feedback value
}
}

void ledTask(void * parameter){
// Get the mqtt message from the queue and process it to led1 and led2
for(;;)
{
   char set_led_state[256];
   if(xQueueReceive(xQueue, &set_led_state,portMAX_DELAY) == pdPASS)
   {
    set_led_state[sizeof(set_led_state) - 1] = '\0';
    xSemaphoreTake(Binary_Semaphore, portMAX_DELAY);
        if(String(set_led_state) == "LED1ON")
        {
            digitalWrite(led1,HIGH);
            EEPROM.write(0,1);
            EEPROM.commit();
        }
    
        else if(String(set_led_state) == "LED1OFF")
        {
            digitalWrite(led1,LOW);
            EEPROM.write(0,0);
            EEPROM.commit();
        }
    
        else if(String(set_led_state) == "LED2ON")
        {
            digitalWrite(led2,HIGH);
            EEPROM.write(1,1);
            EEPROM.commit();
        }
        
        else if(String(set_led_state) == "LED2OFF")
        {
            digitalWrite(led2,LOW);
            EEPROM.write(1,0);
            EEPROM.commit();
        } 
      
    }
    xSemaphoreGive(Binary_Semaphore);
    vTaskDelay(200 / portTICK_PERIOD_MS);
}
}

void loop(){}
