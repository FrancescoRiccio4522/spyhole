#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>

// Dati rete WiFi
const char* ssid = "andrea";
const char* password = "12345678";

// URL del server a cui fare POST
const char* serverUrl = "http://172.20.10.2:5000/upload";

#define LED_PIN 4
// Setup camera per modulo AI Thinker (modifica se usi altro modello)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
void startCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound()){
    config.frame_size = FRAMESIZE_SVGA; // 800x600, qualità migliore

    //config.frame_size = FRAMESIZE_VGA;
    //config.jpeg_quality = 10;
    config.jpeg_quality = 8;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_PIN, OUTPUT);  // <<== Imposta il pin del LED come uscita
  WiFi.begin(ssid, password);
  Serial.print("Connessione WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connesso!");

  startCamera();
  xTaskCreatePinnedToCore(
  uartTask,       // funzione
  "UART Task",    // nome task
  4096,           // stack size
  NULL,           // parametri
  1,              // priorità
  NULL,           // handle
  1               // core (1 = core App, 0 = core Pro)
);

}

void uartTask(void * parameter) {
  Serial.setTimeout(100);  // importante: timeout breve per non bloccare
  while (true) {
    if (Serial.available()) {
      String command = Serial.readStringUntil('\n');
      command.trim();

      if (command == "2") {
        //[DEBUG]Serial.println("Scatto foto...");
        digitalWrite(LED_PIN, HIGH);

        camera_fb_t * fb = esp_camera_fb_get();
        if (!fb || fb->format != PIXFORMAT_JPEG) {
          //[DEBUG]Serial.println("Errore acquisizione!");
          if (fb) esp_camera_fb_return(fb);
          digitalWrite(LED_PIN, LOW);
          continue;
        }

        HTTPClient http;
        http.setTimeout(10000);
        http.begin(serverUrl);
        http.addHeader("Content-Type", "image/jpeg");

        int httpResponseCode = http.POST(fb->buf, fb->len);
        if (httpResponseCode > 0) {
          String response = http.getString();
          //[DEBUG]Serial.println("Server risponde: " + response);
          if (response.indexOf("not ok") ==-1){
            Serial.write('Y');
          }
          else{
          Serial.write('N');
          }
        } else {
          //[DEBUG]Serial.printf("Errore invio POST: %d\n", httpResponseCode);
          Serial.write('N');
        }

        http.end();
        esp_camera_fb_return(fb);
        digitalWrite(LED_PIN, LOW);
      }
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);  // evita uso eccessivo della CPU
  }
}

void loop() {

  //attesa attiva
   delay(100);
}
