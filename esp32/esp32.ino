#include <WiFi.h>
//#include <FirebaseESP32.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include <ESP32Servo.h> // Thư viện điều khiển servo cho ESP32
#include <LiquidCrystal_I2C.h>

// Thông tin kết nối WiFi
//const char* ssid = "Huong Tran_2.4g";
//const char* password = "tughidi9";

const char* ssid = "iPhone của Hiếu";
const char* password = "123456789";


// Cấu hình Firebase
  // Chèn Khóa API của dự án Firebase
  #define API_KEY "AIzaSyBGU1riE5-HtJaZ-fiKoiAaUNhbj0WHTT8"

  // Nhập Tên người dùng được ủy quyền và Mật khẩu tương ứng
  #define USER_EMAIL "hieun6334@gmail.com"
  #define USER_PASSWORD "Zxcv123123@"

  // Thông tin Firebase
  #define FIREBASE_HOST "teamdha-12864-default-rtdb.firebaseio.com"
  #define FIREBASE_AUTH "s2zVUa91xFf3de5FaXQK2Mx9gg6hjnWEEHu4i42k"

  #define DATABASE_URL "https://teamdha-12864-default-rtdb.firebaseio.com/"
  
  // Xác định đối tượng Firebase
  FirebaseData stream;
  FirebaseData fbdo;
  FirebaseAuth auth;
  FirebaseConfig config;
  FirebaseData firebaseData;
  // Biến để lưu đường dẫn cơ sở dữ liệu
  String listenerPath = "/";

  // Cung cấp thông tin về quy trình tạo mã thông báo.
    #include "addons/TokenHelper.h"

    // Cung cấp thông tin in tải trọng RTDB và các chức năng hỗ trợ khác.
    #include "addons/RTDBHelper.h"

// Khai báo chân relay
#define RELAY1 26
#define RELAY2 27
#define RELAY3 25

// Khai báo chân nút cảm ứng TTP223
#define BUTTON1 33
#define BUTTON2 32
#define BUTTON3 35

// Khai báo chân cảm biến mưa và cảm biến khí gas MQ-135
#define RAIN_SENSOR_PIN 34
#define MQ135_PIN 36


// Khai báo còi buzzer
#define BUZZER_PIN 23


// Thiết lập cảm biến DHT11
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Khai báo chân servo
#define SERVO_PIN 13
Servo rainServo;

// Khai báo màn hình LCD 2004
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Biến lưu trạng thái relay
int relay1State = HIGH;
int relay2State = HIGH;
int relay3State = HIGH;

int relay1_OldState = HIGH;
int relay2_OldState = HIGH;
int relay3_OldState = HIGH;

// Biến lưu trạng thái mưa và khí gas
int rainStatus = HIGH;
bool gasDetected = false;

// Biến debounce
bool button1Pressed = false;
bool button2Pressed = false;
bool button3Pressed = false;


/*// Khởi tạo Firebase và WiFiClient
FirebaseData firebaseData;
FirebaseData stream;
FirebaseConfig config;
FirebaseAuth auth;*/


void setup() {
  Serial.begin(115200);
  // Kết nối WiFi
    WiFi.begin(ssid, password);
    Serial.print("Đang kết nối WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nĐã kết nối WiFi!");
    Serial.print("Địa chỉ IP: ");
    Serial.println(WiFi.localIP());

  // Khởi tạo Firebase
    // Chỉ định khóa API (bắt buộc)
    config.api_key = API_KEY;
    // Chỉ định thông tin đăng nhập của người dùng
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
    // Chỉ định URL RTDB (bắt buộc)
    config.database_url = DATABASE_URL;
    Firebase.reconnectWiFi(true);
    // Chỉ định hàm gọi lại cho tác vụ tạo mã thông báo chạy dài
    config.token_status_callback = tokenStatusCallback;  //Xem addons/TokenHelper.h

    // Chỉ định thời gian thử lại tối đa của việc tạo mã thông báo
    config.max_token_generation_retry = 5;

    // Khởi tạo thư viện với Firebase auth và config
    Firebase.begin(&config, &auth);

  // Thiết lập chân relay là OUTPUT
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  digitalWrite(RELAY1, relay1State);
  digitalWrite(RELAY2, relay2State);
  digitalWrite(RELAY3, relay3State);

  // Thiết lập chân nút cảm ứng là INPUT
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  pinMode(BUTTON3, INPUT);

  // Thiết lập cảm biến mưa và khí gas
  pinMode(RAIN_SENSOR_PIN, INPUT);
  pinMode(MQ135_PIN, INPUT);

  // Khởi động còi buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Khởi động cảm biến DHT11
  dht.begin();

  // Thiết lập Servo
  rainServo.attach(SERVO_PIN);
  rainServo.write(0); // Góc ban đầu là 0 độ

  // Khởi động LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(2000);

}

// Biến lưu thời gian
  unsigned long previousMillis = 0;
  unsigned long previousMillis2 = 0;
  unsigned long previousMillis3 = 0;
  unsigned long count = 0;

void loop() {
  // Kiểm tra thời gian hiện tại
  unsigned long currentMillis = millis();

    if (currentMillis - previousMillis2 >= 50) {
    previousMillis2 = currentMillis;
    // Kiểm tra trạng thái nút cảm ứng và điều khiển relay
    checkTouchButtons();
  }

  // Nếu thời gian đã trôi qua đủ 2500 ms
  if (currentMillis - previousMillis >= 2000) {
    previousMillis = currentMillis;
    // Đọc trạng thái cảm biến mưa và điều khiển servo
    rainSensorToFirebase();
    updateGasSensor();
    // Đọc cảm biến DHT và gửi dữ liệu JSON lên Firebase
    sendDataToFirebase();
    // Đọc trạng thái relay từ Firebase thông qua JSON
    fetchDataFromFirebase();
    // Cập nhật hiển thị trên LCD
    updateLCD();
  }
}

void checkTouchButtons() {
  // Nút cảm ứng 1
  if (digitalRead(BUTTON1) == HIGH) {
    if (!button1Pressed) {
      button1Pressed = true;
      relay1State = !relay1State;
      digitalWrite(RELAY1, relay1State);

      FirebaseJson json1;
      json1.add("Relay1", relay1State == HIGH ? "1" : "0");
      if (Firebase.RTDB.updateNode(&fbdo, "/", &json1)) { // Gửi dữ liệu lên Firebase
        Serial.println("Relay 1 cập nhật Firebase thành công!");
      } else {
        Serial.printf("Lỗi cập nhật Firebase: %s\n", fbdo.errorReason().c_str());
      }
    }
  } else {
    button1Pressed = false;
  }

  // Nút cảm ứng 2
  if (digitalRead(BUTTON2) == HIGH) {
    if (!button2Pressed) {
      button2Pressed = true;
      relay2State = !relay2State;
      digitalWrite(RELAY2, relay2State);

      FirebaseJson json2;
      json2.add("Relay2", relay2State == HIGH ? "1" : "0");
      if (Firebase.RTDB.updateNode(&fbdo, "/", &json2)) { // Gửi dữ liệu lên Firebase
        Serial.println("Relay 2 cập nhật Firebase thành công!");
      } else {
        Serial.printf("Lỗi cập nhật Firebase: %s\n", fbdo.errorReason().c_str());
      }
    }
  } else {
    button2Pressed = false;
  }

  // Nút cảm ứng 3
  if (digitalRead(BUTTON3) == HIGH) {
    if (!button3Pressed) {
      button3Pressed = true;
      relay3State = !relay3State;
      digitalWrite(RELAY3, relay3State);

      FirebaseJson json3;
      json3.add("Relay3", relay3State == HIGH ? "1" : "0");
      if (Firebase.RTDB.updateNode(&fbdo, "/", &json3)) { // Gửi dữ liệu lên Firebase
        Serial.println("Relay 3 cập nhật Firebase thành công!");
      } else {
        Serial.printf("Lỗi cập nhật Firebase: %s\n", fbdo.errorReason().c_str());
      }
    }
  } else {
    button3Pressed = false;
  }
}


// Đọc và gửi dữ liệu JSON lên Firebase
void sendDataToFirebase() {
  FirebaseJson json4;

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int currentRainStatus = digitalRead(RAIN_SENSOR_PIN);

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Lỗi đọc DHT11!");
    return;
  }

  json4.add("Temperature", temperature);
  json4.add("Humidity", humidity);
  json4.add("RainSensor", currentRainStatus == LOW ? "0" : "1");
  json4.add("GasSensor", gasDetected ? "1" : "0");

  Serial_Printf("Dữ liệu JSON gửi lên Firebase... %s\n\n", Firebase.RTDB.updateNode(&fbdo, "/", &json4) ? "ok" : fbdo.errorReason().c_str());
        
}

// Đọc trạng thái cảm biến mưa và điều khiển servo
void rainSensorToFirebase() {
  int currentRainStatus = digitalRead(RAIN_SENSOR_PIN);

  if (currentRainStatus != rainStatus) {
    rainStatus = currentRainStatus;

    // Điều khiển servo
    if (rainStatus == LOW) {
      rainServo.write(90); // Quay servo đến 90 độ khi có mưa
      Serial.println("Trời mưa - Servo quay đến 90 độ");
    } else {
      rainServo.write(0); // Quay servo về 0 độ khi trời không mưa
      Serial.println("Trời không mưa - Servo quay về 0 độ");
    }

    // Gửi trạng thái lên Firebase
    //Firebase.setString(firebaseData, "/RainSensor", rainStatus == LOW ? "0" : "1");
    Serial.println("Cập nhật trạng thái mưa lên Firebase");
  }
}

// Hàm đọc dữ liệu JSON từ Firebase và điều khiển relay
void fetchDataFromFirebase() {
  // Lấy dữ liệu JSON từ Firebase
  if (Firebase.RTDB.getJSON(&fbdo, "/")) {
    FirebaseJson &json = fbdo.jsonObject(); // Lấy đối tượng JSON
    FirebaseJsonData json1, json2, json3;   // Ba biến để chứa dữ liệu từ JSON

    // Đọc giá trị từ JSON cho Relay1
    if (json.get(json1, "Relay1")) {
      String relay1StateFromFirebase = json1.stringValue; // Lấy giá trị dưới dạng chuỗi
      Serial.print("Relay 1 từ Firebase (JSON): ");
      Serial.println(relay1StateFromFirebase);
      relay1State = relay1StateFromFirebase == "1" ? HIGH : LOW;
      digitalWrite(RELAY1, relay1State);
    } else {
      Serial.println("Không tìm thấy giá trị Relay1 trong JSON!");
    }

    // Đọc giá trị từ JSON cho Relay2
    if (json.get(json2, "Relay2")) {
      String relay2StateFromFirebase = json2.stringValue; // Lấy giá trị dưới dạng chuỗi
      Serial.print("Relay 2 từ Firebase (JSON): ");
      Serial.println(relay2StateFromFirebase);
      relay2State = relay2StateFromFirebase == "1" ? HIGH : LOW;
      digitalWrite(RELAY2, relay2State);
    } else {
      Serial.println("Không tìm thấy giá trị Relay2 trong JSON!");
    }

    // Đọc giá trị từ JSON cho Relay3
    if (json.get(json3, "Relay3")) {
      String relay3StateFromFirebase = json3.stringValue; // Lấy giá trị dưới dạng chuỗi
      Serial.print("Relay 3 từ Firebase (JSON): ");
      Serial.println(relay3StateFromFirebase);
      relay3State = relay3StateFromFirebase == "1" ? HIGH : LOW;
      digitalWrite(RELAY3, relay3State);
    } else {
      Serial.println("Không tìm thấy giá trị Relay3 trong JSON!");
    }
  } else {
    Serial.print("Lỗi lấy JSON từ Firebase: ");
    Serial.println(fbdo.errorReason());
  }
}

// Hàm cập nhật hiển thị trên LCD
void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.printf("Nhiet do: %.1f C", dht.readTemperature());
  lcd.setCursor(0, 1);
  lcd.printf("Do am: %.1f %%", dht.readHumidity());
  lcd.setCursor(0, 2);
  lcd.print(rainStatus == LOW ? "Thoi Tiet:co Mua" : "Thoi Tiet:Kho Rao");
  lcd.setCursor(0, 3);
  lcd.print(gasDetected ? "Khi Gas: Co" : "Khi Gas: Khong");
}

// Hàm cập nhật trạng thái cảm biến khí gas
void updateGasSensor() {
  gasDetected = digitalRead(MQ135_PIN) == HIGH;

  if (gasDetected) {
    digitalWrite(BUZZER_PIN, HIGH); // Bật còi
    Serial.println("Phát hiện khí gas! Buzzer kích hoạt.");
  } else {
    digitalWrite(BUZZER_PIN, LOW); // Tắt còi
  }
}
