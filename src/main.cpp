// Includes global variables and librarys that the OLED display uses
#include "main.h"
#include <FBJS_Config.h>
#include <FirebaseJson.h>

#define FIREBASEJSON_USE_PSRAM

#define WIFI_NAME "TalTech"
#define WIFI_PASSWORD ""

// Change it according to the real name of the microcontroller where DHT shield is connected
#define MQTT_ROOT_TOPIC_PREFIX "ESP36"
#define FILE_UPLOAD_TOPIC_PREFIX "file"

// OLED reset pin is GPIO0
#define OLED_RESET 0

UploadSession uploadData = {0};
unsigned long lastUpdateTime = 0;
int currentFileIdx = 0;

// Message received
void iot_received(String topic, String msg)
{
  // Handle init message
  if (topic == (MQTT_ROOT_TOPIC_PREFIX "/" FILE_UPLOAD_TOPIC_PREFIX "/upload/start/init"))
  {
    uploadData = *handleUploadInitMessage(msg);
    if (uploadData.status_flag)
    {
      Serial.println("Upload session initialized successfully:");
      Serial.print("Session ID: ");
      Serial.println(uploadData.session_id);
      Serial.print("Total Size (MB): ");
      Serial.println(uploadData.total_size_mb);
      Serial.print("Number of Files: ");
      Serial.println(uploadData.num_of_files);
      Serial.print("Speed (MB/s): ");
      Serial.println(uploadData.speed_mbps);
      for (int i = 0; i < uploadData.num_of_files; i++)
      {
        Serial.print("File ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(uploadData.files[i].file_name.c_str());
        Serial.print(" (");
        Serial.print(uploadData.files[i].file_size_mb);
        Serial.println(" MB)");
      }
      uploadData.session_state.value = CONNECTING;
      uploadData.session_state.connected_flag = false;
    }
    else
    {
      Serial.println("Failed to initialize upload session");
    }
  }
  // Handle READY message from ESP2
  else if (topic.indexOf("/upload/") > 0 && topic.endsWith("/ready"))
  {
    Serial.println("Received READY from ESP2: " + msg);
    if (msg == "READY" && uploadData.session_state.value == CONNECTING)
    {
      Serial.println("Starting upload simulation...");
      uploadData.session_state.value = UPLOADING;
      lastUpdateTime = millis();
      currentFileIdx = 0;
    }
  }
}

// Function started after the connection to the server is established.
void iot_connected()
{
  // Send message to serial port to show that connection is established
  Serial.println("MQTT connected callback");

  // Subscribe to init topic
  String initTopic = String(MQTT_ROOT_TOPIC_PREFIX) + "/" + FILE_UPLOAD_TOPIC_PREFIX + "/upload/start/init";
  iot.subscribe(initTopic.c_str());
  Serial.print("Subscribed to: ");
  Serial.println(initTopic);

  // Send message to MQTT server to show that connection is established
  iot.log("Upload reporter started!");
}

void setup()
{
  // Initialize serial port and send message
  Serial.begin(115200); // setting up serial connection parameter
  Serial.println("Booting");

  // Initialize upload data structure
  uploadData.total_size_mb = 0;
  uploadData.uploaded_mb = 0;
  uploadData.num_of_files = 0;
  uploadData.status_flag = false;
  uploadData.speed_mbps = 10.0;
  uploadData.files = nullptr;
  uploadData.session_state.value = IDLE;
  uploadData.session_state.connected_flag = false;
  strcpy(uploadData.session_id, "");

  // https://registry.platformio.org/libraries/mobizt/FirebaseJson

  iot.setConfig("wname", WIFI_NAME);
  iot.setConfig("wpass", WIFI_PASSWORD);
  iot.setConfig("msrv", "193.40.245.72");
  iot.setConfig("mport", "1883");
  iot.setConfig("muser", "test");
  iot.setConfig("mpass", "test");
  iot.printConfig(); // print IoT json config to serial
  iot.setup();       // Initialize IoT library
}

void loop()
{
  iot.handle(); // IoT behind the plan work, it should be periodically called
  handleUploadStateMachine();

  delay(200);
}

UploadSession *handleUploadInitMessage(String message)
{

  FirebaseJson json;
  UploadSession *session = new UploadSession;

  json.setJsonData(message);

  // session id is a timestamp in millis
  String sessionId = String(millis());
  strcpy(session->session_id, sessionId.c_str());

  FirebaseJsonData data;
  json.get(data, "total_size_mb");
  if (!data.success)
  {
    Serial.println("Failed to parse total_size_mb from message");
    session->status_flag = false;
    return session;
  }
  session->total_size_mb = data.to<float>();

  json.get(data, "num_of_files");
  if (!data.success)
  {
    Serial.println("Failed to parse num_of_files from message");
    session->status_flag = false;
    return session;
  }
  session->num_of_files = data.to<int>();

  // optional speed_mbps, default to 10 MB/s
  json.get(data, "speed_mbps");
  if (data.success)
  {
    session->speed_mbps = data.to<float>();
  }
  else
  {
    session->speed_mbps = 10.0; // Default speed
  }

  if (!generateFileUploadData(session))
  {
    Serial.println("Failed to generate file upload data");
    session->status_flag = false;
    return session;
  }

  session->status_flag = true;
  return session;
}

bool generateFileUploadData(UploadSession *session)
{
  session->files = new FileUploadData[session->num_of_files];
  float *weights = new float[session->num_of_files];
  float total_weight = 0;
  randomSeed(millis());

  for (int i = 0; i < session->num_of_files; i++)
  {
    weights[i] = random(50, 201);
    total_weight += weights[i];
  }

  float cumulative_size_mb = 0;
  char buffer[256];
  for (int i = 0; i < session->num_of_files - 1; i++)
  {
    session->files[i].file_size_mb = (weights[i] / total_weight) * session->total_size_mb;
    snprintf(buffer, sizeof(buffer), "file_%d.dat", i + 1);
    session->files[i].file_name = buffer;
    session->files[i].uploaded_mb = 0.0;
    cumulative_size_mb += session->files[i].file_size_mb;
    buffer[0] = '\0';
  }
  // Set the size of the last file to the remaining size
  session->files[session->num_of_files - 1].file_size_mb = session->total_size_mb - cumulative_size_mb;
  snprintf(buffer, sizeof(buffer), "file_%d.dat", session->num_of_files);
  session->files[session->num_of_files - 1].file_name = buffer;
  session->files[session->num_of_files - 1].uploaded_mb = 0.0;

  delete[] weights;
  return true;
}

void sendConfigToESP2()
{
  FirebaseJson json;
  json.add("total_size_mb", uploadData.total_size_mb);
  json.add("num_of_files", uploadData.num_of_files);
  json.add("session_id", String(uploadData.session_id));

  String configMsg;
  json.toString(configMsg, true);

  String topic = String(FILE_UPLOAD_TOPIC_PREFIX) + "/upload/start";
  iot.publishMsg(topic.c_str(), configMsg.c_str());

  Serial.println("Sent config to ESP2:");
  Serial.println(configMsg);

  String readyTopic = "+/" + String(MQTT_ROOT_TOPIC_PREFIX) + "/" + FILE_UPLOAD_TOPIC_PREFIX + "/upload/" + String(uploadData.session_id) + "/ready";
  iot.subscribe(readyTopic.c_str());
  Serial.print("Subscribed to: ");
  Serial.println(readyTopic);

  uploadData.session_state.connected_flag = true;
}

void sendFileStatus(int fileIndex, float deltaUploadedMb)
{
  if (fileIndex < 0 || fileIndex >= uploadData.num_of_files)
  {
    return;
  }

  FileUploadData &file = uploadData.files[fileIndex];

  FirebaseJson json;
  json.add("file_name", file.file_name.c_str());
  json.add("uploaded_mb", file.uploaded_mb);
  json.add("total_size", file.file_size_mb);
  json.add("delta_uploaded_mb", deltaUploadedMb);

  String statusMsg;
  json.toString(statusMsg, true);

  String topic = String(FILE_UPLOAD_TOPIC_PREFIX) + "/upload/" + String(uploadData.session_id) + "/status";
  iot.publishMsg(topic.c_str(), statusMsg.c_str());

  Serial.print("Status: ");
  Serial.println(statusMsg);
}

void handleUploadSessionEnd()
{
  String topic = String(FILE_UPLOAD_TOPIC_PREFIX) + "/upload/" + String(uploadData.session_id) + "/end";
  iot.publishMsg(topic.c_str(), "Upload complete");
}

void handleUploadStateMachine()
{
  switch (uploadData.session_state.value)
  {
  case IDLE:
    // do nothing
    break;

  case CONNECTING:
    // send config to the other ESP with LED matrix
    if (!uploadData.session_state.connected_flag)
    {
      sendConfigToESP2();
    }
    // check iot_received for READY message handling
    break;

  case UPLOADING:
  {
    // elapsed is used to limit the frequency of status updates
    unsigned long currentTime = millis();
    unsigned long elapsedMs = currentTime - lastUpdateTime;

    if (elapsedMs >= FILE_REPORT_STATUS_FREQUENCY_MS)
    {
      float mbUplaoded = (elapsedMs / 1000.0) * uploadData.speed_mbps;

      FileUploadData &currentFile = uploadData.files[currentFileIdx];
      currentFile.uploaded_mb += mbUplaoded;

      // for file completion to trigger on the ESP1, uploaded file size must be equal to file size
      if (currentFile.uploaded_mb >= currentFile.file_size_mb)
      {
        currentFile.uploaded_mb = currentFile.file_size_mb;
      }

      // regular status report
      sendFileStatus(currentFileIdx, mbUplaoded);

      // if current file is complete
      if (currentFile.uploaded_mb >= currentFile.file_size_mb)
      {
        char logMsg[128];
        sprintf(logMsg, "File %d uploaded: %s (%.2f/%.2f MB)", currentFileIdx + 1, currentFile.file_name.c_str(), currentFile.uploaded_mb, currentFile.file_size_mb);
        iot.log(logMsg);

        currentFileIdx++;

        // session end
        if (currentFileIdx >= uploadData.num_of_files)
        {
          iot.log("All files uploaded! Returning to IDLE");
          handleUploadSessionEnd();
          uploadData.session_state.value = IDLE;
          uploadData.session_state.connected_flag = false;
          currentFileIdx = 0;

          // cleanup
          if (uploadData.files != nullptr)
          {
            delete[] uploadData.files;
            uploadData.files = nullptr;
          }
        }
      }

      lastUpdateTime = currentTime;
    }
  }
  break;

  default:
    break;
  }
}
