
#include "MQ135.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "ESP32_MailClient.h"



const char* ssid = "VODAFONE_1540";
const char* password = "159ad157OK20coal";
WiFiClient client;


#define emailSenderAccount    "<email>"
#define emailSenderPassword   "<password>"
#define smtpServer            "smtp.gmail.com"
#define smtpServerPort        465
#define emailSubject          "[ALERT] ESP32 "
#define emailRecipient        "omer.atila@ceng.deu.edu.tr"
SMTPData smtpData;
void sendCallback(SendStatus info);

#include <ThingSpeak.h>

const long CHANNEL = 1268023;
const char * WRITE_API = "W8JDC9QLBUMKUDVX";
const char* server = "api.thingspeak.com";

const int sampleWindow = 50;                              // Sample window width in mS (50 mS = 20Hz)
unsigned int sample;

#define SENSOR_PIN 34
#define GasSENSOR 35
#define DELAY 500

MQ135 gasSensor = MQ135(GasSENSOR);

int sensorValue = 0;

void setup(void) {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  ThingSpeak.begin(client);


}

void loop(void) {
  unsigned long startMillis = millis();
  float peakToPeak = 0;                                  // peak-to-peak level

  unsigned int signalMax = 0;                            //minimum value
  unsigned int signalMin = 1024;                         //maximum value

  // collect data for 50 mS
  while (millis() - startMillis < sampleWindow)
  {
    sample = analogRead(SENSOR_PIN);                    //get reading from microphone
    if (sample < 1024)                                  // toss out spurious readings
    {
      if (sample > signalMax)
      {
        signalMax = sample;                           // save just the max levels
      }
      else if (sample < signalMin)
      {
        signalMin = sample;                           // save just the min levels
      }
    }
  }

  peakToPeak = signalMax - signalMin;                    // max - min = peak-peak amplitude
  int db = map(peakToPeak, 136, 950, 30, 85);         //calibrate for deciBels

  float ppm = gasSensor.getPPM();
  Serial.print("Value: ");
  Serial.println(ppm);
  ThingSpeak.setField(1, ppm);
  ThingSpeak.setField(3, db);
  ThingSpeak.writeFields(CHANNEL, WRITE_API);

  if (ppm > 1000 || db > 80) {
    smtpData.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);
    smtpData.setSender("ESP32", emailSenderAccount);
    smtpData.setPriority("High");
    smtpData.setSubject(emailSubject);
    smtpData.addRecipient(emailRecipient);
    if (ppm > 1000)
      smtpData.setMessage("<div style=\"color:#2f4468;\"><h1>ppm is over 1000. it is: </h1>" + String(ppm)+ "<p>- Sent from ESP32 board</p></div>", true);
    else
      smtpData.setMessage("<div style=\"color:#2f4468;\"><h1>db is over 80. it is: </h1>" + String(db) + "<p>- Sent from ESP32 board</p></div>", true);
    smtpData.setSendCallback(sendCallback);
    MailClient.sendMail(smtpData);
    smtpData.empty();
    delay(30000);
  }

  delay(5000);      // thingspeak needs minimum 15 sec delay between updates.
}
void sendCallback(SendStatus msg) {
  // Print the current status
  Serial.println(msg.info());

  // Do something when complete
  if (msg.success()) {
    Serial.println("----------------");
  }
}
