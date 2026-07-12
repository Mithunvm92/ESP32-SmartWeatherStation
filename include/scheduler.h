#pragma once
// ============================================================================
// scheduler.h — Cooperative, non-blocking millis()-based task scheduler
//
// Replaces delay() everywhere. Register tasks with an interval and a
// callback; call Scheduler::run() once per loop() iteration. Each task
// runs at most once per its interval, and a slow task never blocks others.
// ============================================================================

#include <Arduino.h>
#include <functional>

using TaskCallback = std::function<void()>;

class Scheduler {
public:
    static constexpr uint8_t MAX_TASKS = 16;

    // Register a recurring task. Returns task ID, or -1 if table is full.
    int8_t addTask(const char* name, uint32_t intervalMs, TaskCallback callback, bool runImmediately = false);

    // Force a task to run on the next run() call regardless of elapsed time.
    void triggerNow(int8_t taskId);

    // Enable/disable a task without removing it.
    void setEnabled(int8_t taskId, bool enabled);

    // Change a task's interval at runtime.
    void setInterval(int8_t taskId, uint32_t intervalMs);

    // Call once per loop() — dispatches any due tasks.
    void run();

private:
    struct Task {
        const char*  name        = nullptr;
        uint32_t     intervalMs  = 0;
        uint32_t     lastRunMs   = 0;
        bool         enabled     = true;
        bool         forceRun    = false;
        TaskCallback callback    = nullptr;
        bool         inUse       = false;
    };

    Task tasks_[MAX_TASKS];
    uint8_t taskCount_ = 0;
};
