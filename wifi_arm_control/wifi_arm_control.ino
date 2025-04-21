// WİFİ, web server ve json kütüphaneleri
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

//Servo kontrol kütüphanesi
#include <Servo.h>

// komut dizesi oluşturmak için gerekli kütüphane
#include <vector>
#include <array>

//Sıcaklık sensörü kütüphaneleri
#include <OneWire.h>
#include <DallasTemperature.h>

// mesefa sensörünü asenkron okumak için kütüphane
#include <NewPing.h>


///////////////////////////////////////  Web server  değişkenleri ve ip sabitleme /////////////////////////////////////////////////
const char *ssid = "GVD";            // bağlanılacak wifi aği adı
const char *password = "123somePass";  // bağlanılacak wifi şifresi


IPAddress local_IP(192, 168, 1, 184);         // Cihazın sabit IP adresi
IPAddress gateway(192, 168, 1, 1);            // Ağ geçidi (genellikle router IP'si)
IPAddress subnet(255, 255, 255, 0);           // Alt ağ maskesi
AsyncWebServer server(80);                    // web sunucusunun başlatılacağı port


/////////////////////////////////////////////  Mosfet pinleri tanımlamarı //////////////////////////////////////////////////////////
int servoPowerTrigPin = 5;    // servo güç pini
int fanPowerTrigPin = 16;     // fan güç pini


////////////////////////////////////////////  Servo değişkenleri  //////////////////////////////////////////////////////////////
// servoların max ve min pulse with değerleri
#define SERVO_MIN_PULSE 544
#define SERVO_MAX_PULSE 2400

Servo servos[6];                                              // 6 adet servo motor için dizi
int servoPins[6] = { 4, 0, 2, 12, 13, 15 };                   // GPIO numaraları Servo motorların bağlı olduğu pinler
int servoAngles[6] = { 120, 90, 3, 4, 105, 90 };              //Servo başlangıç açıları sırayla
int servoAnglesMicroSeconds[6] = { 0, 0, 0, 0, 0, 0 };        // Mevcut pozisyonlar mikro saniye cinsinden
int servoTargetAnglesMicroSeconds[6] = { 0, 0, 0, 0, 0, 0 };  // Hedef pozisyonlar mikrosaniye cinsinden

// servoların manuel kontrollerini ivmelendirmek için gerekli değişkenler 
int desiredDirection[6] = { 0 };  // 1: ileri, -1: geri, 0: yok
unsigned long lastPressTime[6] = { 0 };
unsigned long pressStartTime[6] = { 0 };
const unsigned long pressTimeout = 150;  // 100ms'de bir gelmezse bırakılmış say


// Tüm Komutları Saklayan Vektör
std::vector<std::array<int, 6>> currentCommands;
int currentCommandIndex = 0;
bool isMoving = false;

// sadece api ucundan doğrudan hareket komutunda kullanılır
float speed = 0.2;          // Başlangıçta hareket hızı
float acceleration = 0.04;  // Hızlanma miktarı

// Servo Pid değer Objesi
struct PID {
  float kp;
  float ki;
  float kd;
};

// servoların pid değerleri
PID servoPID[6] = {
  /*  Servo-1  */ { 0.8, 0.0, 0.01 },

  /*  Servo-2  */ { 0.4, 0.0, 0.01 },

  /*  Servo-3  */ { 0.4, 0.0, 0.008 },

  /*  Servo-4  */ { 0.4, 0.0, 0.004 },

  /*  Servo-5  */ { 0.4, 0.0, 0.01 },

  /*  Servo-6  */ { 0.4, 0.0, 0.004 }
};

//dt dinamik ve hedef sapması değişmeden önceki değerler
// PID servoPID[6] = {
//   /*  Servo-1  */ { 0.8, 0.0, 0.01 },

//   /*  Servo-2  */ { 0.1, 0.01, 0.02 },

//   /*  Servo-3  */ { 0.2, 0.8, 0.01 },

//   /*  Servo-4  */ { 0.2, 0.02, 0.01 },

//   /*  Servo-5  */ { 0.2, 0.0, 0.01 },

//   /*  Servo-6  */ { 0.2, 0.08, 0.0001 }
// };

float prevErrors[6] = { 0, 0, 0, 0, 0, 0 };          // Önceki hata
float integrals[6] = { 0, 0, 0, 0, 0, 0 };           // Integral değerleri
const unsigned long pidInterval = 20;                // pid hesaplama sıklığı
unsigned long lastPIDTime = 0;                       // son yapılan pid hesaplaması millis cinsinden
float dt = pidInterval / 1000.0;  // saniye  // 2 hesaplama arasındaki fark pid hesaplamada kullanılacak olan


////////////////////////////////////////////  Sensör değişkenleri  //////////////////////////////////////////////////////////////
int tempSensorPin = 14;       // sıcaklık sensörü veri pini
const int trigPin = 1;        // mesafe sensörü trig pini TX pini kullanıldı
const int echoPin = 3;        // mesafe sensörü echo pini RX pini kullanıldı
const int maxDistance = 300;  // cm  max okuma mesafesi
NewPing sonar(trigPin, echoPin, maxDistance);

float tempValue = 0;      // sıcaklık değeri
float distanceValue = 0;  // uzaklık değeri
unsigned long lastDistanceRead = 0;
unsigned long lastTempRequest = 0;
bool tempRequested = false;

OneWire oneWire(tempSensorPin);       // Sıcaklık sensörü tanımlaması
DallasTemperature sensors(&oneWire);  // hesaplamarı yapan kütüphane


void setup() {
  Serial.begin(115200);  // seri haberleşme başlatma // production da kapatılmalı

  pinMode(servoPowerTrigPin, OUTPUT);    // servo power pini OUTPUT olarak tanımlandı
  digitalWrite(servoPowerTrigPin, LOW);  // servo power pini LOW seviyesine çekildi

  pinMode(fanPowerTrigPin, OUTPUT);    // fan power pini OUTPUT olarak tanımlandı
  digitalWrite(fanPowerTrigPin, LOW);  // fan power pini LOW seviyesine çekildi

  if(Serial.available()){
    pinMode(trigPin, OUTPUT);  // mesafe sensörü trig pini OUTPUT olarak tanımlandı
    pinMode(echoPin, INPUT);   // mesafe sensörü echo pini INPUT olarak tanımlandı
  }
 
  // Servolar tanımlandı ve başlangıç açılarına gönderildi
  for (int i = 0; i < 6; i++) {
    servos[i].attach(servoPins[i], SERVO_MIN_PULSE, SERVO_MAX_PULSE);
    moveServoSmooth(i + 1, servoAngles[i]);
  }

  // ESP8266 nın bootu için 2sn beklendi
  delay(2000);

  // Wifi bağlantısı yapılacak
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("WiFi bağlanıyor..");
  }

  // ESP'nin local adresi seri monitöre yazdırılıyor
  Serial.println(WiFi.localIP());

  // Kontrol uçlarını tanıtıyoruz
  registerControlRoutes();

  // Web sunucumuzu başlatıyoruz
  server.begin();

  // servolara gücü veriyoruz
  digitalWrite(servoPowerTrigPin, HIGH);

  // sıcaklık sensörünü başlatıyoruz
  sensors.begin();
}


void loop() {
  unsigned long now = millis();
  dt = (now - lastPIDTime) / 1000.0;
  readSensors(now);
  manuelControlCheck(now);
  pidCheck(now);
}
