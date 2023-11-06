
// WiFi
const char *ssid = "Sebbyy"; // Enter your WiFi name
const char *password = "Ayokongasabi@123";  // Enter WiFi password

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const char *topic = "led3_status";
const char *mqtt_username = "yuenicdao";
const char *mqtt_password = "yuenicdao";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);


void callback(char *topic, byte *payload, unsigned int length) {
 Serial.print("Message arrived in topic: ");
 Serial.println(topic);
 Serial.print("Message:");
 for (int i = 0; i < length; i++) {
     Serial.print((char) payload[i]);
 }
 Serial.println();
 Serial.println("-----------------------");
}


void setup() {
 // Set software serial baud to 115200;
 Serial.begin(9600);
 // connecting to a WiFi network
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.println("Connecting to WiFi..");
 }
 Serial.println("Connected to the WiFi network");
 //connecting to a mqtt broker
 client.setServer(mqtt_broker, mqtt_port);
 client.setCallback(callback);
 while (!client.connected()) {
     String client_id = "esp32-client-";
     client_id += String(WiFi.macAddress());
     Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
     if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
         Serial.println("Public emqx mqtt broker connected");
     } else {
         Serial.print("failed with state ");
         Serial.print(client.state());
         delay(2000);
     }
 }
 // publish and subscribe
 client.publish(topic, "Hi EMQX I'm ESP32 ^^");
 client.subscribe(topic);
}


void loop() {
 client.loop();
}

// Function prototypes for our tasks
void task1Function(void *pvParameters);
void task2Function(void *pvParameters);
void task3Function(void *pvParameters);

SemaphoreHandle_t Binary_Semaphore;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // Create the tasks
  xTaskCreate(task1Function, "Task1", 10000, NULL, 1, NULL);
  xTaskCreate(task2Function, "Task2", 10000, NULL, 2, NULL);
  xTaskCreate(task3Function, "Task3", 10000, NULL, 3, NULL);

  // Start the FreeRTOS scheduler
  vTaskStartScheduler();
}

void loop() {
  // This loop should be empty since FreeRTOS is running the tasks
}

void task1Function(void *pvParameters) {
  (void)pvParameters; // Parameter not used

  for (;;) {
    Serial.println("Task 1 is running");
    vTaskDelay(1000/portTICK_PERIOD_MS) // Delay for 1 second
  }
}

void task2Function(void *pvParameters) {
  (void)pvParameters; // Parameter not used

  for (;;) {
    Serial.println("Task 2 is running");
    vTaskDelay(pdMS_TO_TICKS(500)); // Delay for 0.5 seconds
  }
}

void task3Function(void *pvParameters) {
  (void)pvParameters; // Parameter not used

  for (;;) {
    Serial.println("Task 3 is running");
    vTaskDelay(pdMS_TO_TICKS(2000)); // Delay for 2 seconds
  }
}



