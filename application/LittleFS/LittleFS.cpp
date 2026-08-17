#include "LittleFS.h"
#include "esp_log.h" // Include ESP-IDF logging

static const char* TAG = "LittleFS_Wrapper";

LittleFSFS LittleFS;

LittleFSFS::LittleFSFS() : basePath_(NULL), partitionLabel_(NULL) {}

LittleFSFS::~LittleFSFS() {
    if (basePath_) free(basePath_);
    if (partitionLabel_) free(partitionLabel_);
}

bool LittleFSFS::begin(bool formatOnFail, const char *basePath, uint8_t maxOpenFiles, const char *partitionLabel) {
    (void)maxOpenFiles;

    if (partitionLabel_) free(partitionLabel_);
    partitionLabel_ = partitionLabel ? strdup(partitionLabel) : NULL;

    if (basePath_) free(basePath_);
    basePath_ = basePath ? strdup(basePath) : NULL;

    if (esp_littlefs_mounted(partitionLabel_)) {
        ESP_LOGW(TAG, "LittleFS Already Mounted!"); // Updated
        return true;
    }

    esp_vfs_littlefs_conf_t conf = {}; 
    conf.base_path = basePath_;
    conf.partition_label = partitionLabel_;
    conf.format_if_mount_failed = false;
    conf.grow_on_mount = true;

    esp_err_t err = esp_vfs_littlefs_register(&conf);
    
    if (err == ESP_FAIL && formatOnFail) {
        if (format()) {
            err = esp_vfs_littlefs_register(&conf);
        }
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Mounting LittleFS failed! Error: %d", err); // Updated
        return false;
    }

    return true;
}

bool LittleFSFS::format() {
    // Removed Arduino watchdog calls
    esp_err_t err = esp_littlefs_format(partitionLabel_);
    if (err) {
        ESP_LOGE(TAG, "Formatting LittleFS failed! Error: %d", err); // Updated
        return false;
    }
    return true;
}

File LittleFSFS::open(const char* fileName, const char* mode) {
    if (!basePath_ || !fileName) return File(NULL);

    size_t total_length = strlen(fileName) + strlen(basePath_) + 1;
    char* fullpath = new char[total_length];
    
    strcpy(fullpath, basePath_);
    strcat(fullpath, fileName);

    FILE* fp = fopen(fullpath, mode);
    delete[] fullpath; 
    return File(fp);
}

bool LittleFSFS::exists(const char* fileName) {
    if (!basePath_ || !fileName) return false;

    // Construct the absolute VFS path
    size_t total_length = strlen(fileName) + strlen(basePath_) + 1;
    char* fullpath = new char[total_length];
    
    strcpy(fullpath, basePath_);
    strcat(fullpath, fileName);

    // Use stat to check if the file/directory exists
    struct stat st;
    bool file_exists = (stat(fullpath, &st) == 0);
    
    delete[] fullpath; // Prevent memory leaks
    return file_exists;
}