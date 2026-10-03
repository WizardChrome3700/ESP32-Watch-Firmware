#ifndef LITTLEFS_H
#define LITTLEFS_H

#include "esp_littlefs.h"
#include <stdio.h>
#include <cstring>
#include <memory>
#include <sys/stat.h>
#include <dirent.h>
#include <string>
// #include "esp_log.h" // Needed for log_e, log_w
// #include "Arduino.h" // Needed for disableCore0WDT()

class File {
private:
    // shared_ptr with a custom deleter ensures fclose is called automatically 
    // when the last copy of this File object is destroyed.
    std::shared_ptr<FILE> _fp;
    std::shared_ptr<DIR> _dp;
    std::string _path;
    std::string _name;

public:
    // Constructor handles the raw pointer and assigns the custom deleter (fclose)
    File() {}

    // Constructor for files
    File(FILE* fp, const char* path) : _path(path ? path : "") {
        if (fp) {
            _fp = std::shared_ptr<FILE>(fp, [](FILE* f) { fclose(f); });
        }
        extractName();
    }

    // Constructor for directories
    File(DIR* dp, const char* path) : _path(path ? path : "") {
        if (dp) {
            _dp = std::shared_ptr<DIR>(dp, [](DIR* d) { closedir(d); });
        }
        extractName();
    }

    void extractName() {
        size_t pos = _path.find_last_of('/');
        if (pos != std::string::npos) {
            _name = _path.substr(pos + 1); // Get everything after the last '/'
        } else {
            _name = _path;
        }
    }

    File openNextFile(const char* mode = "r") {
        if (!_dp) return File(); // Not a directory stream
        
        struct dirent* entry;
        while ((entry = readdir(_dp.get())) != nullptr) {
            // Ignore '.' and '..'
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            std::string childPath = _path;
            if (childPath.back() != '/') childPath += '/';
            childPath += entry->d_name;

            // Blindly open as a file since you guaranteed no subdirectories!
            // Remove the dangerous cast!
            FILE* fp = fopen(childPath.c_str(), mode);
            return File(fp, childPath.c_str());
        }
        return File(); // No more files
    }

    size_t write(uint8_t c) {
        if (!_fp) return 0;
        return fwrite(&c, 1, 1, _fp.get());
    }

    size_t write(uint8_t* buffer, size_t bufferSize) {
        if (!_fp) return 0;
        return fwrite(buffer, 1, bufferSize, _fp.get());
    }

    size_t read(uint8_t* buffer, size_t bufferSize) {
        if (!_fp) return 0;
        return fread(buffer, 1, bufferSize, _fp.get());
    }

    void close() {
        // Resetting the shared_ptr automatically triggers the fclose lambda
        _fp.reset(); 
    }
    
    // Check if file is actually open (useful for: if(file) { ... })
    operator bool() const {
        return (_fp != nullptr) || (_dp != nullptr); // ADD _dp check
    }

    // Move the read/write pointer to a specific location
    bool seek(long offset, int mode = SEEK_SET) {
        if (!_fp) return false;
        // fseek returns 0 on success
        return fseek(_fp.get(), offset, mode) == 0; 
    }

    // Get the current position of the read/write pointer
    size_t position() const {
        if (!_fp) return 0;
        return ftell(_fp.get());
    }

    // Get the total size of the file in bytes
    size_t size() const {
        if (!_fp) return 0;
        
        // Save the current position
        long currentPos = ftell(_fp.get()); 
        
        // Jump to the end of the file to read the size
        fseek(_fp.get(), 0, SEEK_END);
        long fileSize = ftell(_fp.get());
        
        // Jump back to where we started
        fseek(_fp.get(), currentPos, SEEK_SET); 
        
        return fileSize;
    }

    const char* name() const {
        return _name.c_str();
    }
};

class LittleFSFS {
private:
    char* basePath_;
    char* partitionLabel_;

public:
    LittleFSFS();
    ~LittleFSFS(); // Destructor needed to clean up strdup memory

    bool begin(bool formatOnFail = false, const char *basePath = "/littlefs", uint8_t maxOpenFiles = 10, const char *partitionLabel = "spiffs");
    bool format();
    File open(const char* fileName, const char* mode);
    bool exists(const char* fileName);
};

// --- Implementation (Usually goes in a .cpp file) ---

extern LittleFSFS LittleFS;

#endif