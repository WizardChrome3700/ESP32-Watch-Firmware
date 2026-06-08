#include "SystemCtrl.h"

void setup() {
    SystemCtrl sysctl(15000);
    sysctl.init();
    sysctl.boot_handler();
    sysctl.system_loop();
    sysctl.shutdown_handler();
}

void loop() {

}