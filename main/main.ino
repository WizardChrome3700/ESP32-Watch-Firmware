#include "SystemCtrl.h"

SystemCtrl sysctl(15000);

void setup() {
    Serial.begin(115200);
    sysctl.init();
    sysctl.boot_handler();
    sysctl.system_loop();
    sysctl.shutdown_handler();
}

void loop() {

}