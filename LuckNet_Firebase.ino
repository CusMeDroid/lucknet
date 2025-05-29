/*
	Caranya
	------------------------------------------------------------------------
	1. Unduh dan daftar aplikasi disini https://luck-net.web.app
	2. Buka aplikasi dan klik ikon tombol setelan dikiri atas & tempel UID anda
	4. Unduh libraries https://github.com/CusMeDroid/Arduino
	3. Hubungkan papan dan unggah
	Selamat mencoba
	------------------------------------------------------------------------
	~ Suryo Dwijayanto
*/

#include <ESP8266WiFi.h>
#include <KSmart_LCDI2C.h>
#include <KSmart_Firebase.h>
#include <DHT.h>

KSmart_LCDI2C lcd(0x27, 16, 2); // SCL D1 SDA D2

#define KSmart_SSID "LuckNet" // Penambatan WiFi SSID
#define KSmart_PASSWORD "Demo12345678" // Penambatan WiFi Kata Sandi

#define DHTPIN D3
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define KSmart_RELAY D0

// Pin untuk sensor kelembaban tanah (sesuaikan dengan pin analog yang Anda gunakan)
const int soilMoisturePin = A0;

#define KSmart_URL "https://luck-net-default-rtdb.asia-southeast1.firebasedatabase.app/"

// ----------------- UID --------------------
String KSmart_UID = "TEMPEL_UID_ANDA_DISINI";
// ----------------- UID --------------------

String KSmart_UNIQ = "users/" + KSmart_UID;

Firebase firebase(KSmart_URL);

void setup() {
  lcd.init();
  lcd.noBacklight();
  
  Serial.begin(115200);
  pinMode(KSmart_RELAY, OUTPUT);
  digitalWrite(KSmart_RELAY, 0);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  // delay(1000);

  // Connect to WiFi
  Serial.println("");
  Serial.println("");
  Serial.print("Connecting to: ");
  Serial.println(KSmart_SSID);
  WiFi.begin(KSmart_SSID, KSmart_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("-");
  }

  Serial.println("");
  Serial.println("WiFi Connected!");
  
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(firebase.getString(KSmart_UNIQ + "/title"));
  lcd.setCursor(0, 1);
  lcd.print(firebase.getString(KSmart_UNIQ + "/description"));
}

String data_relay;
String data_auto;
String xon = "on";
String xoff = "off";
String xTitle;
    
void loop() {
  lcd.init();
  
  // Baca data dari sensor DHT
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature(); // Anda mungkin perlu readTemperature(unit) untuk Celcius/Fahrenheit
  
  // Periksa kesalahan pembacaan
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    // Kirim data ke Firebase
    firebase.setString(KSmart_UNIQ + "/humidity", String(humidity));
    firebase.setString(KSmart_UNIQ + "/temperature", String(temperature));
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
  }
  
  data_relay = firebase.getString(KSmart_UNIQ + "/relay");
  if (data_relay == "on") {
    digitalWrite(KSmart_RELAY, 1);
    xTitle = firebase.getString(KSmart_UNIQ + "/title")+" - ON ";
    Serial.println("Relay On");
  } else if (data_relay == "off") {
    digitalWrite(KSmart_RELAY, 0);
    xTitle = firebase.getString(KSmart_UNIQ + "/title")+" - OFF ";
    Serial.println("Relay Off");
  }

  // Baca data dari sensor kelembaban tanah
  int soilMoistureValue = analogRead(soilMoisturePin);

  // Mapping nilai analog ke rentang 0-100%
  float soilMoisturePercentage = map(soilMoistureValue, 0, 1023, 0, 100);

  // Kirim data ke Firebase
  firebase.setFloat(KSmart_UNIQ + "/soilmoisture", soilMoisturePercentage);

  Serial.print("Soil Moisture Analog: ");
  Serial.print(soilMoisturePercentage);
  Serial.print(" %\t");
  
  // Auto On
  data_auto = firebase.getString(KSmart_UNIQ + "/auto");
  if (data_auto == "on"){
    Serial.println("Set auto on");
    String data_sm_value = firebase.getString(KSmart_UNIQ + "/sm_value");
    int xxdtr = data_sm_value.toInt();
    Serial.println(xxdtr);
    // Kontrol Relay berdasarkan data ANALOG (jika data digital tidak reliabel)
    if (soilMoisturePercentage < xxdtr) { // Sesuaikan ambang batas kelembaban
      digitalWrite(KSmart_RELAY, 0); // Relay OFF
      xTitle = firebase.getString(KSmart_UNIQ + "/title")+" - OFF ";
      firebase.setString(KSmart_UNIQ + "/relay", String(xoff));
      Serial.println("Relay OFF (Tanah Basah - Data Analog)");
    } else if (soilMoisturePercentage > xxdtr) {
      digitalWrite(KSmart_RELAY, 1); // Relay ON
      xTitle = firebase.getString(KSmart_UNIQ + "/title")+" - ON ";
      firebase.setString(KSmart_UNIQ + "/relay", String(xon));
      Serial.println("Relay ON (Tanah Kering - Data Analog)");
    }
  } else if (data_auto == "off") {
    Serial.println("Set auto off");
  }
  
  lcd.setCursor(0, 0);
  lcd.print(xTitle);
  lcd.setCursor(0, 1);
  
  // Format floats for LCD using sprintf
  char buffer[16]; // Adjust size as needed
  sprintf(buffer, "%d%% %dC %d%%", (int)humidity, (int)temperature, (int)soilMoisturePercentage);
  lcd.print(buffer);
}