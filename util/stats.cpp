//
// Created by daiyi on 2019/09/30.
//

#include <cassert>
#include <x86intrin.h>
#include "stats.h"
#include "time.h"
#include <cstring>

using std::stoull;

namespace adgMod {
    int MOD = 0;
    bool string_mode = true;
    uint32_t model_error = 10;
    int block_restart_interval = 16;
    uint32_t test_num_level_segments = 100;
    uint32_t test_num_file_segments = 100;
    int key_size;
    int value_size;

    class Timer {
        friend class Stats;

        void Start();
        void Pause();
        void Reset();
        uint64_t Time();

        struct timespec time_started;
        uint64_t timestamp_accumulated;
        bool started;

    public:
        Timer();
        ~Timer() = default;
    };

    Timer::Timer() : timestamp_accumulated(0), started(false) {}

    void Timer::Start() {
        assert(!started);
        clock_gettime(CLOCK_MONOTONIC, &time_started);
        started = true;
    }

    void Timer::Pause() {
        assert(started);
        struct timespec time_ended;
        clock_gettime(CLOCK_MONOTONIC, &time_ended);
        uint64_t time_elapse = (time_ended.tv_sec - time_started.tv_sec) * 1000000000 + time_ended.tv_nsec - time_started.tv_nsec;
        timestamp_accumulated += time_elapse;
        started = false;
    }

    void Timer::Reset() {
        timestamp_accumulated = 0;
        started = false;
    }

    uint64_t Timer::Time() {
        assert(!started);
        return timestamp_accumulated;
    }

    Stats* Stats::singleton = nullptr;

    Stats::Stats() : timers(9), level_stats(9, 0) {}

    Stats* Stats::GetInstance() {
        if (!singleton) singleton = new Stats();
        return singleton;
    }

    void Stats::StartTimer(uint32_t id) {
        for (size_t i = timers.size(); i < id + 1; ++i) timers.push_back(Timer{});

        Timer& timer = timers[id];
        timer.Start();
    }

    void Stats::PauseTimer(uint32_t id) {
        Timer& timer = timers[id];
        timer.Pause();
    }

    void Stats::ResetTimer(uint32_t id) {
        Timer& timer = timers[id];
        timer.Reset();
    }

    uint64_t Stats::ReportTime(uint32_t id) {
        Timer& timer = timers[id];
        return timer.Time();
    }

    void Stats::ReportTime() {
        for (int i = 0; i < timers.size(); ++i) {
            printf("Timer %u: %lu\n", i, timers[i].Time());
        }
    }



    void Stats::RecordLevel(int level) {
        for (size_t i = level_stats.size(); i < level + 1; ++i) level_stats.push_back(0);

        level_stats[level]++;
    }

    void Stats::ReportLevelStats() {
        for (int i = 0; i < level_stats.size(); ++i) {
            printf("Level %d: %u\n", i, level_stats[i]);
        }
    }

    void Stats::ResetAll() {
        timers.clear();
        level_stats.clear();
    }

    Stats::~Stats() {
        ReportTime();
    }




    void LearnedIndexData::AddSegment(string&& x, uint64_t y) {
        if (string_mode) {
            string_segments.push_back(std::make_pair(std::move(x), y));
        } else {
            segments.push_back(std::make_pair(stoull(x), y));
        }
    }

    uint64_t LearnedIndexData::GetPosition(const string& target_x) const {
        uint64_t target_int = stoull(target_x);

        if (string_mode) {
            assert(string_segments.size() >= 2);
            if (target_x > string_segments.back().first) return string_segments.back().second + 1;
            if (target_x < string_segments.front().first) return string_segments.back().second + 1;

            uint32_t left = 0, right = (uint32_t) string_segments.size() - 1;
            while (left != right - 1) {
                assert(left < right);
                uint32_t mid = (right + left) / 2;
                if (target_x < string_segments[mid].first) right = mid;
                else left = mid;
            }

            uint64_t x1 = stoull(string_segments[left].first), x2 = stoull(string_segments[right].first);
            uint64_t y1 = string_segments[left].second, y2 = string_segments[right].second;
            return y1 + (y2 - y1) * (target_int - x1) / (x2 - x1);
        } else {
            assert(segments.size() >= 2);
            if (target_int > segments.back().first) return segments.back().second + 1;
            if (target_int < segments.front().first) return segments.back().second + 1;

            uint32_t left = 0, right = (uint32_t) segments.size() - 1;
            while (left != right - 1) {
                assert(left < right);
                uint32_t mid = (right + left) / 2;
                if (target_int < segments[mid].first) right = mid;
                else left = mid;
            }

            uint64_t x1 = segments[left].first, x2 = segments[right].first;
            uint64_t y1 = segments[left].second, y2 = segments[right].second;
            return y1 + (y2 - y1) * (target_int - x1) / (x2 - x1);
        }
    }

    uint64_t LearnedIndexData::MaxPosition() const {
        return string_mode ? string_segments.back().second : segments.back().second;
    }

    uint32_t LearnedIndexData::GetError() const {
        return error;
    }

    void LearnedIndexData::AddKey(string&& key) {
        string_keys.push_back(key);
    }

    void LearnedIndexData::Learn() {
        return;
    }





    string ExtractString(const char* pos, size_t size) {
        return std::move(string(pos, size));
    }

    uint64_t ExtractInteger(const char* pos, size_t size) {
        char* temp = new char[size + 1];
        memcpy(temp, pos, size);
        temp[size] = '\0';
        uint64_t result = (uint64_t) atol(temp);
        delete[] temp;
        return result;
    }

    bool SearchNumEntriesArray(const std::vector<uint64_t>& num_entries_array, const uint64_t pos,
                                size_t* index, uint64_t* relative_pos) {
        size_t left = 0, right = num_entries_array.size() - 1;
        while (left < right) {
            size_t mid = (left + right) / 2;
            if (pos < num_entries_array[mid]) right = mid;
            else left = mid + 1;
        }
        *index = left;
        *relative_pos = left > 0 ? pos - num_entries_array[left - 1] : pos;
        return left < num_entries_array.size();
    }


    string generate_key(uint64_t key) {
        string key_string = to_string(key);
        string result = string(key_size - key_string.length(), '0') + key_string;
        return std::move(result);
    }

    string generate_value(uint64_t value) {
        string value_string = to_string(value);
        string result = string(value_size - value_string.length(), '0') + value_string;
        return std::move(result);
    }
}
