//============================LIBRARIES USED===============================
//https://github.com/lemmingDev/ESP32-BLE-Gamepad
//https://github.com/oferzv/wifiTool

//bluetooth
#include <BleConnectionStatus.h>
#include <BleGamepad.h>

//internet
#include <wifiTool.h>
#include <HTTPClient.h>

//to handle json
#include "ArduinoJson.h"

//===========================DEFINE PINS START=============================

//buttons
#define BUTTON_PIN1 2
#define BUTTON_PIN2 16
#define BUTTON_PIN3 4
#define BUTTON_PIN4 0
#define BUTTON_PIN5 33
#define BUTTON_PIN6 27
#define BUTTON_PIN7 15
#define BUTTON_PIN8 32
#define BUTTON_PIN9 14
#define BUTTON_PIN10 12
#define BUTTON_PIN11 13

//LEDs
#define BATTERY_LED_PIN 23
#define BLUETOOTH_LED_PIN 26
#define WIFI_LED_PIN 25
//============================DEFINE PINS END==============================

//========================VARIABLE DECLARING START=========================

//wifi variables
WifiTool wifiTool;
String networkURL = "http://298cfedd929d.ngrok.io";

//setting name and manufacturer of the bluetooth controller
BleGamepad bleGamepad("IOT Controller Project", "Yosri Jerbi");

//used when accessing buttons
int pin[12] = {0,
               BUTTON_PIN1,
               BUTTON_PIN2,
               BUTTON_PIN3,
               BUTTON_PIN4,
               BUTTON_PIN5,
               BUTTON_PIN6,
               BUTTON_PIN7,
               BUTTON_PIN8,
               BUTTON_PIN9,
               BUTTON_PIN10,
               BUTTON_PIN11
              };
int previousButtonState[12] = {HIGH, HIGH, HIGH, HIGH,
                               HIGH, HIGH, HIGH, HIGH,
                               HIGH, HIGH, HIGH, HIGH
                              };
uint64_t buttonMap[12] = {(1 << 0), (1 << 0), (1 << 1), (1 << 2),
                          (1 << 3), (1 << 4), (1 << 5), (1 << 6),
                          (1 << 7), (1 << 8), (1 << 9), (1 << 10)};

//dummy database values/default button map
int defaultButtons[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

//used for mapping buttons from Serial Monitor
//(1 << 0) = bitshifting
uint64_t customMap = (1 << 0);
int customButton = 0;
bool next = false;

//used for checking button combinations
int combo1Check = 0;
int combo2Check = 0;
bool combo1CheckTrigger = false;
bool combo2CheckTrigger = false;

//used for cheking LED states
bool bluetoothCheckTrigger = false;
bool powerCheckTrigger = false;
//=========================VARIABLE DECLARING END==========================

//===========================MAIN METHODS START============================
void setup() {
  Serial.begin(115200);
  wifiSetup();
  setButtonPinModes();
  bleSetup();
}
void loop() {
  checkPower();
  serialMapping();
  bluetoothBehaviour();
}
//============================MAIN METHODS END=============================

//===========================SETUP METHODS START===========================
void wifiSetup() {
  wifiTool.begin(false);
  if (!wifiTool.wifiAutoConnect()) {
    Serial.println("failed to connect");
    wifiTool.runApPortal();
  }
}
void setButtonPinModes() {
  for (int i = 1; i < 12; i++) {
    pinMode(pin[i], INPUT_PULLUP);
  }
  pinMode(BLUETOOTH_LED_PIN, OUTPUT);
  pinMode(WIFI_LED_PIN, OUTPUT);
  pinMode(BATTERY_LED_PIN, OUTPUT);
}
void bleSetup() {
  Serial.println("Starting BLE work!");
  bleGamepad.begin();
  Serial.println("The device started, now you can pair it with bluetooth!");
  setButtonsFromDB(defaultButtons);
}
//===========================SETUP METHODS END=============================

//===========================LOOP METHODS START============================
void checkPower() {
  if (!powerCheckTrigger) {
    powerCheckTrigger = true;
    digitalWrite(BATTERY_LED_PIN, HIGH);
  }
}

char inputBuffer[16];
unsigned int val;

void serialMapping() {
  //allow for mapping buttons through serial monitor
  if (Serial.available() > 0) {
    // A function that reads characters from the serial port into a buffer.
    Serial.readBytes(inputBuffer, sizeof(inputBuffer));

    // Convert string to integer
    // cplusplus.com/reference/cstdlib/atoi/
    val = atoi(inputBuffer);

    // memset clears buffer and updates string length so strlen(inputBuffer) is accurate.
    memset(inputBuffer, 0, sizeof(inputBuffer));

    if (val > 0) {

      int number = val - 1;
      customMap = ((1 << number));
      customButton = value;
      next = true;

      Serial.print("press button you wish to assign to button"); Serial.println(value, DEC);
    }
  }
}

void bluetoothBehaviour() {
  if (bleGamepad.isConnected())
  {
    //turn on blue LED when connected to bluetooth
    if (!bluetoothCheckTrigger) {
      bluetoothCheckTrigger = true;
      digitalWrite(BLUETOOTH_LED_PIN, HIGH);
    }
    for (int i = 1; i <= 11; i++) {
      readControllerButton(i);
    }
    //if a specific button combo is pressed check for a new button map from the database
    if (combo1Check == 4 ) {
      combo1CheckTrigger = true;
    } else if (combo1Check == 0) {
      if (combo1CheckTrigger) {
        wifiLEDBlink(3);
        getButtonMapFromDB();
        combo1CheckTrigger = false;
      }
      digitalWrite(BATTERY_LED_PIN, HIGH);
    }
    //if a specific button combo is pressed save the new button map to the database
    if (combo2Check == 4 ) {
      combo2CheckTrigger = true;
    } else if (combo2Check == 0) {
      if (combo2CheckTrigger) {
        wifiLEDBlink(3);
        saveButtonMapToDB();
        combo2CheckTrigger = false;
      }
      digitalWrite(BATTERY_LED_PIN, HIGH);
    }
  } else {
    if (bluetoothCheckTrigger) {
      bluetoothCheckTrigger = false;
      digitalWrite(BLUETOOTH_LED_PIN, LOW);
    }
  }
}
//===========================LOOP METHODS END==============================

//=========================WIFI RELATED METHODS============================

//blink the wifi LED x amount of times
void wifiLEDBlink(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(WIFI_LED_PIN, HIGH);
    delay(100);
  }
}

void saveButtonMapToDB() {
  Serial.print("saving buttons to DB\n");
  if (wifiTool.wifiAutoConnect()) {
    digitalWrite(WIFI_LED_PIN, HIGH);
    HTTPClient http;
    Serial.print("[HTTP] begin...\n");
    String values = String("gamepadProfileID=") + "99998" +
                    "&button1=" + defaultButtons[1] +
                    "&button2=" + defaultButtons[2] +
                    "&button3=" + defaultButtons[3] +
                    "&button4=" + defaultButtons[4] +
                    "&button5=" + defaultButtons[5] +
                    "&button6=" + defaultButtons[6] +
                    "&button7=" + defaultButtons[7] +
                    "&button8=" + defaultButtons[8] +
                    "&button9=" + defaultButtons[9] +
                    "&button10=" + defaultButtons[10] +
                    "&button11=" + defaultButtons[11];
    Serial.print("[HTTP] send values...\n");
    Serial.print(networkURL);
    Serial.print("/put_db.php?");
    Serial.println(values);
    http.begin(networkURL + "/put_db.php?" + values); //HTTP
    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        //String payload = http.getString();
        Serial.println("HTTP WORKED");
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    delay(1000);
    http.end();
    digitalWrite(WIFI_LED_PIN, LOW);
    Serial.print("done saving buttons to DB\n");
  }
}
void getButtonMapFromDB() {
  if (wifiTool.wifiAutoConnect()) {
    digitalWrite(WIFI_LED_PIN, HIGH);
    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    // configure traged server and url
    //http.begin("https://www.howsmyssl.com/a/check", ca); //HTTPS
    http.begin(networkURL + "/get_db.php?gamepadProfileID=99999"); //HTTP

    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println(payload);

        StaticJsonDocument<384> doc;

        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          return;
        }

        const char* id = doc["id"]; // "99999"

        JsonArray jsonButtons = doc["buttons"];
        int parsedJsonButtons[12] = {jsonButtons[0],
                                     jsonButtons[1],
                                     jsonButtons[2],
                                     jsonButtons[3],
                                     jsonButtons[4],
                                     jsonButtons[5],
                                     jsonButtons[6],
                                     jsonButtons[7],
                                     jsonButtons[8],
                                     jsonButtons[9],
                                     jsonButtons[10],
                                     jsonButtons[11]
                                    };

        setButtonsFromDB(parsedJsonButtons);

      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    delay(1000);
    http.end();
    digitalWrite(WIFI_LED_PIN, LOW);
  }
}

void setButtonsFromDB(int buttons[12]) {
  Serial.println("Getting latest buttonmap from DB");
  for (int i = 1; i < 12; i++) {
    defaultButtons[i] = buttons[i];
    Serial.print("button ");
    Serial.print(i);
    Serial.print("(pin ");
    Serial.print(pin[i]);
    Serial.print(") >> ");
    Serial.println(buttons[i]);
    //    buttons[i]//current buttonmap
    uint64_t mappedButton = (1 << buttons[i] - 1);
    buttonMap[i] = mappedButton;
  }
  Serial.println("Done mapping buttons");
}

//==========================CONTROLLER BUTTONS=============================

void readControllerButton(int buttonIndex) {
  int currentButtonState = digitalRead(pin[buttonIndex]);
  if (currentButtonState != previousButtonState[buttonIndex])
  {
    if (next) {
      next = false;
      defaultButtons[buttonIndex] = customButton;
      buttonMap[buttonIndex] = customMap;
    }
    if (currentButtonState == LOW)
    {
      bleGamepad.press(buttonMap[buttonIndex]);
      Serial.print("Button: ");
      Serial.print(buttonIndex);
      Serial.print(" ||output: ");
      Serial.println(buttonMap[buttonIndex]);

      checkCombo1(buttonIndex, 1);
      checkCombo2(buttonIndex, 1);
    }
    else
    {
      bleGamepad.release(buttonMap[buttonIndex]);
      checkCombo1(buttonIndex, -1);
      checkCombo2(buttonIndex, -1);
    }
  }
  previousButtonState[buttonIndex] = currentButtonState;
}

void checkCombo1(int index, int value) {
  if (index == 7 || index == 8 || index == 1 || index == 10) {
    combo1Check += value;
  }
};
void checkCombo2(int index, int value) {
  if (index == 7 || index == 8 || index == 1 || index == 9) {
    combo2Check += value;
  }
};
