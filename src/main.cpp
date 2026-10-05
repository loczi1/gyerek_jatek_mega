#include <Arduino.h>
#include "app/Application.h"

void setup() {
    application::begin();
}

void loop() {
    application::run();
}