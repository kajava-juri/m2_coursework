#pragma once
#include <string>

#define FILE_REPORT_STATUS_FREQUENCY_MS 200

enum UploadState {
  IDLE,
  CONNECTING,
  UPLOADING
};

struct UploadSessionState {
    UploadState value;
    bool connected_flag;
};

struct UploadSession {
    float total_size_mb;
    float uploaded_mb;
    int num_of_files;
    char session_id[128];
    bool status_flag;
    float speed_mbps;
    FileUploadData* files;
    UploadSessionState session_state;
} uploadData;

struct FileUploadData {
    std::string file_name;
    float file_size_mb;
    float uploaded_mb;
} fileUploadData;

// void handleUploadInitialization(UploadSession& session) {
//     // Initialize upload data
//     session.total_size_mb = 0.0;
//     session.uploaded_mb = 0.0;
//     session.num_of_files = 0;
//     strcpy(session.session_id, "");
// }

UploadSession* handleUploadInitMessage(String message);
bool generateFileUploadData(UploadSession* session);
void handleUploadStateMachine();
void sendConfigToESP2();
void sendFileStatus(int fileIndex);
