#pragma once

#include <chrono>
#include <thread>
#include <vector>
#include <string>

inline float getTicksMs() {
    using Clock = std::chrono::steady_clock;
    static auto zero = Clock::now();
    auto t = Clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(t - zero).count() / 1000.0f;
}

inline void wait(int numMs) { std::this_thread::sleep_for(std::chrono::milliseconds(numMs)); }

inline int randBetween(int minVal, int maxVal) { return minVal + rand() % (maxVal - minVal); }

enum class ActivityType {
    eat,
    eatFailure,
    think,
    leave,
};

class PhilosopherEventLog {
public:
    PhilosopherEventLog(const char* philosopherName)
        : philosopherName_(philosopherName) {}

    //! Called when a philosopher starts an activity
    void startActivity(ActivityType at) {
        auto t = getTicksMs();
        events_.emplace_back(Event{at, int(t), true});
        // printf("%8.3f: %s start %s\n", t, philosopherName_.c_str(), activityToString(at));
    }
    //! Called when a philosopher ends an activity
    void endActivity(ActivityType at) {
        auto t = getTicksMs();
        events_.emplace_back(Event{at, int(t), false});
        // printf("%8.3f: %s end %s\n", t, philosopherName_.c_str(), activityToString(at));
    }

    void printSummary(int stepMs = 5) const {
        printf("%15s: ", philosopherName_.c_str());
        char curFill = ' ';
        int lastTimestamp = 0;
        for (const auto& ev : events_) {
            int numCharsToFill = ev.timestamp_/stepMs - lastTimestamp/stepMs;
            printChars(curFill, numCharsToFill);

            lastTimestamp = ev.timestamp_;
            if (ev.isStart_)
                curFill = activityToChar(ev.activityType_);
            else
                curFill = ' ';
        }
        printChars(curFill, 1);
        printf("\n");
    }

private:
    static char activityToChar(ActivityType at) {
        switch (at) {
        case ActivityType::eat:
            return 'E';
        case ActivityType::eatFailure:
            return '.';
        case ActivityType::think:
            return 't';
        case ActivityType::leave:
            return 'L';
        default:
            return '?';
        }
    }
    static const char* activityToString(ActivityType at) {
        switch (at) {
        case ActivityType::eat:
            return "eat";
        case ActivityType::eatFailure:
            return "failed to eat";
        case ActivityType::think:
            return "think";
        case ActivityType::leave:
            return "leave";
        default:
            return "???";
        }
    }
    static void printChars(char ch, int count) {
        for (int i = 0; i < count; i++)
            putchar(ch);
    }

    std::string philosopherName_;
    struct Event {
        ActivityType activityType_;
        int timestamp_;
        bool isStart_;
    };
    std::vector<Event> events_;
};