#pragma once
// ============================================================================
// ota_manager.h — ArduinoOTA setup and progress reporting
// ============================================================================

#include <Arduino.h>
#include <functional>

class OtaManager {
public:
    // onProgress(percent) is invoked during an update so the display can
    // show a progress screen. onStart/onEnd let the display manager pause
    // normal rendering while a flash is in progress.
    void begin(std::function<void()> onStart,
               std::function<void(unsigned int)> onProgress,
               std::function<void()> onEnd);

    void handle();   // call every loop() iteration — non-blocking when idle
    bool isUpdating() const { return updating_; }

private:
    bool updating_ = false;
};
