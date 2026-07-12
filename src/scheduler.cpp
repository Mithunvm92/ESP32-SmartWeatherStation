#include "scheduler.h"
#include "logger.h"

static const char* TAG = "Scheduler";

int8_t Scheduler::addTask(const char* name, uint32_t intervalMs, TaskCallback callback, bool runImmediately) {
    if (taskCount_ >= MAX_TASKS) {
        Logger::error(TAG, String("Task table full, cannot add: ") + name);
        return -1;
    }

    for (uint8_t i = 0; i < MAX_TASKS; ++i) {
        if (!tasks_[i].inUse) {
            tasks_[i].name       = name;
            tasks_[i].intervalMs = intervalMs;
            tasks_[i].callback   = callback;
            tasks_[i].enabled    = true;
            tasks_[i].inUse      = true;
            // Stagger first run slightly so not everything fires on boot together,
            // unless the caller explicitly wants an immediate run.
            tasks_[i].lastRunMs  = runImmediately ? 0 : millis();
            tasks_[i].forceRun   = runImmediately;
            taskCount_++;
            Logger::debug(TAG, String("Registered task: ") + name + " every " + intervalMs + " ms");
            return static_cast<int8_t>(i);
        }
    }
    return -1;
}

void Scheduler::triggerNow(int8_t taskId) {
    if (taskId < 0 || taskId >= MAX_TASKS || !tasks_[taskId].inUse) return;
    tasks_[taskId].forceRun = true;
}

void Scheduler::setEnabled(int8_t taskId, bool enabled) {
    if (taskId < 0 || taskId >= MAX_TASKS || !tasks_[taskId].inUse) return;
    tasks_[taskId].enabled = enabled;
}

void Scheduler::setInterval(int8_t taskId, uint32_t intervalMs) {
    if (taskId < 0 || taskId >= MAX_TASKS || !tasks_[taskId].inUse) return;
    tasks_[taskId].intervalMs = intervalMs;
}

void Scheduler::run() {
    const uint32_t now = millis();

    for (uint8_t i = 0; i < MAX_TASKS; ++i) {
        Task& t = tasks_[i];
        if (!t.inUse || !t.enabled || !t.callback) continue;

        // Handles millis() overflow (~49.7 days) safely via unsigned subtraction.
        const bool due = t.forceRun || (now - t.lastRunMs >= t.intervalMs);
        if (!due) continue;

        t.lastRunMs = now;
        t.forceRun  = false;

        // Each task is expected to be non-blocking. Any exceptions/crashes
        // inside a task are the task's own responsibility to guard against;
        // the watchdog protects the overall system from a runaway task.
        t.callback();
    }
}
