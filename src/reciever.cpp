#include <ittiot.h>
#include <Adafruit_GFX.h>
#include <WEMOS_Matrix_GFX.h>
#include <SPI.h>
#include <Adafruit_I2CDevice.h>
#include <FBJS_Config.h>
#include <FirebaseJson.h>
#include "main.h"

#define WIFI_NAME "TalTech"
#define WIFI_PASSWORD ""

#define FILEREPORTER "ESP36"

int row1Progress = 0;   // 1st row = current file progress
int row3Progress = 0;   // 3rd row = overall progress

float total_size_mb = 0;
int num_of_files = 0;
char session_id[128] = {0};

float total_uploaded_mb = 0;
float current_file_uploaded = 0;
float current_file_size = 0;

bool connected_flag = false;

UploadState state = IDLE;

// function prototypes
void handleFileStatus();
void handleUploadStateMachine();

MLED matrix(7); // LED matrix brightness

// ===============================
// MQTT MESSAGE RECEIVER
// ===============================
void iot_received(String topic, String msg)
{
  FirebaseJson json;
  FirebaseJsonData data;

  // Receive upload configuration
  if (topic == (FILEREPORTER "/file/upload/start"))
  {
    json.setJsonData(msg);

    Serial.println("Message received");

    json.get(data, "total_size_mb");
    total_size_mb = data.to<float>();

    json.get(data, "num_of_files");
    num_of_files = data.to<int>();

    json.get(data, "session_id");
    String sid = data.to<String>();
    strcpy(session_id, sid.c_str());

    // reset session data
    row1Progress = 0;
    row3Progress = 0;
    total_uploaded_mb = 0;
    current_file_uploaded = 0;
    current_file_size = 0;
    connected_flag = false;

    state = CONNECTING;
  }

  // Receive upload progress messages
  else if (topic.indexOf("/upload/") > 0 && topic.endsWith("/status"))
  {
    json.setJsonData(msg);

    json.get(data, "uploaded_mb");
    current_file_uploaded = data.to<float>();

    json.get(data, "total_size");
    current_file_size = data.to<float>();

    handleFileStatus();
  }
}

// ===============================
// MQTT CONNECTED CALLBACK
// ===============================
void iot_connected()
{
  Serial.println("MQTT connected");

  String startTopic = String(FILEREPORTER) + "/file/upload/start";
  iot.subscribe(startTopic.c_str());

  String statusTopic = String(FILEREPORTER) + String("/file/upload/+/status");
  iot.subscribe(statusTopic.c_str());

  Serial.println("Subscribed to upload topics");
}

// ===============================
// SETUP
// ===============================
void setup()
{
  Serial.begin(115200);
  Serial.println("LED Matrix Upload Receiver");

  iot.setConfig("wname", WIFI_NAME);
  iot.setConfig("wpass", WIFI_PASSWORD);
  iot.setConfig("msrv", "193.40.245.72");
  iot.setConfig("mport", "1883");
  iot.setConfig("muser", "test");
  iot.setConfig("mpass", "test");

  iot.printConfig();
  iot.setup();
}

// ===============================
// LOOP
// ===============================
void loop()
{
  iot.handle();
  handleUploadStateMachine();
  delay(50);
}

// ===============================
// STATE MACHINE
// ===============================
void handleUploadStateMachine()
{
  switch (state)
  {
    case IDLE:
      matrix.clear();
      matrix.writeDisplay();
      break;

    case CONNECTING:
      if (!connected_flag)
      {
        String topic = String(FILEREPORTER) +
                       "/file/upload/" +
                       String(session_id) +
                       "/ready";

        iot.publishMsg(topic.c_str(), "READY");

        connected_flag = true;
        Serial.println("READY message sent");
      }

      state = UPLOADING;
      break;

    case UPLOADING:
      matrix.clear();

      // 1st row = current file progress
      for (int x = 0; x < row1Progress; x++)
      {
        matrix.drawPixel(x, 0, LED_ON);
      }

      // 3rd row = overall progress
      for (int x = 0; x < row3Progress; x++)
      {
        matrix.drawPixel(x, 2, LED_ON);
      }

      matrix.writeDisplay();
      break;

    default:
      break;
  }
}

// ===============================
// PROGRESS VISUALIZATION
// ===============================
void handleFileStatus()
{
  float currentProgress = 0;
  float overallProgress = 0;
// current file progress
  if (current_file_size > 0)
  {
    currentProgress = current_file_uploaded / current_file_size;
  }

  row1Progress = (int)(currentProgress * 8);

  if (row1Progress > 8)
  {
    row1Progress = 8;
  }

  // overall progress
  total_uploaded_mb = 0;

  // if uploaded_mb is cumulative across all files, use it directly instead:
  // overallProgress = current_file_uploaded / total_size_mb;

  // if uploaded_mb is only current file progress, then you need external logic
  // to track previously completed files correctly.
  // For now assume current progress contributes to total:
  total_uploaded_mb = current_file_uploaded;

  if (total_size_mb > 0)
  {
    overallProgress = total_uploaded_mb / total_size_mb;
  }

  row3Progress = (int)(overallProgress * 8);

  if (row3Progress > 8)
  {
    row3Progress = 8;
  }
}