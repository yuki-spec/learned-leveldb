//
// Created by daiyi on 2019/09/30.
//

#include <cassert>
#include "stats.h"
#include <ctime>
#include <cmath>
#include "plr.h"

using std::stoull;

namespace adgMod {
    int MOD = 0;
    bool string_mode = true;
    uint64_t key_multiple = 1;
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




    void LearnedIndexData::AddSegment(string&& x, double k, double b) {
        if (string_mode) {
            string_segments.push_back(Segment(x, k, b));
        } else {
            assert(false);
        }
    }

    std::pair<uint64_t, uint64_t> LearnedIndexData::GetPosition(const Slice& target_x) const {
        if (string_mode) {
            if (adgMod::MOD == 3) {
                if (target_x > string_keys.back()) return std::make_pair(string_keys.size(), string_keys.size());
                if (target_x < string_keys.front()) return std::make_pair(string_keys.size(), string_keys.size());

                uint32_t left = 0, right = (uint32_t) string_keys.size() - 1;
                while (left < right) {
                    uint32_t mid = (right + left) / 2;
                    int comp = compare(target_x, string_keys[mid]);
                    if (comp < 0) right = mid - 1;
                    else if (comp > 0) left = mid + 1;
                    else {
                        return std::make_pair(mid, mid);
                    }
                }
                return std::make_pair(left, left);
            }

            else {
                assert(string_segments.size() > 1);

                uint64_t target_int = SliceToInteger(target_x);
                if (target_x > string_keys.back()) return std::make_pair(string_keys.size(), string_keys.size());
                if (target_x < string_keys.front()) return std::make_pair(string_keys.size(), string_keys.size());

                uint32_t left = 0, right = (uint32_t) string_segments.size() - 1;
                while (left != right - 1) {
                    uint32_t mid = (right + left) / 2;
                    if (target_x < string_segments[mid].x) right = mid;
                    else left = mid;
                }

                double result = target_int * string_segments[left].k + string_segments[left].b;
                uint64_t lower = result - error> 0 ? (uint64_t) std::floor(result - error) : 0;
                uint64_t upper = (uint64_t) std::ceil(result + error);

//                printf("%s %s %s\n", string_keys[lower].c_str(), string(target_x.data(), target_x.size()).c_str(), string_keys[upper].c_str());
//                assert(target_x >= string_keys[lower] && target_x <= string_keys[upper]);
                return std::make_pair(lower, upper);
            }
        } else {
//            uint64_t target_int = SliceToInteger(target_x);
//            if (target_int > segments.back().first) return segments.back().second + 1;
//            if (target_int < segments.front().first) return segments.back().second + 1;
//
//            uint32_t left = 0, right = (uint32_t) segments.size() - 1;
//            while (left != right - 1) {
//                uint32_t mid = (right + left) / 2;
//                if (target_int < segments[mid].first) right = mid;
//                else left = mid;
//            }
//
//            uint64_t x1 = segments[left].first, x2 = segments[right].first;
//            uint64_t y1 = segments[left].second, y2 = segments[right].second;
//            return y1 + (y2 - y1) * (target_int - x1) / (x2 - x1);
            return std::make_pair(0, 0);
        }
    }

    uint64_t LearnedIndexData::MaxPosition() const {
        return string_keys.size() - 1;
    }

    double LearnedIndexData::GetError() const {
        return error;
    }

    void LearnedIndexData::Learn() {
        // FILL IN GAMMA (error)
        PLR plr = PLR(error);

        if (string_keys.empty()) return;

        std::vector<struct point> points;
        int i = 0;
        for (string &s : string_keys) {
            struct point p{ .x = (double) stoull(s), .y = (double) i};
            points.push_back(p);
            ++i;
        }

        std::vector<struct segment> segs = plr.train(points);
        for (struct segment &s : segs) {
            string_segments.push_back(Segment(generate_key((uint64_t) s.start), s.slope, s.intercept));
        }
        string_segments.push_back(Segment(string_keys.back(), 0, 0));

        for (auto& str: string_segments) {
            //printf("%s %f\n", str.first.c_str(), str.second);
        }
        return;
    }




    void AccumulatedNumEntriesArray::Add(uint64_t num_entries, string &&key) {
        array.push_back(std::make_pair(num_entries, key));
    }

    bool AccumulatedNumEntriesArray::Search(const Slice& key, uint64_t lower, uint64_t upper, size_t* index,
                                             uint64_t* relative_lower, uint64_t* relative_upper) {
        if (adgMod::MOD == 4) {
            uint64_t lower_pos = lower / array[0].first;
            uint64_t upper_pos = upper / array[0].first;
            if (lower_pos != upper_pos) {
                while (true) {
                    if (lower_pos >= array.size()) return false;
                    if (key <= array[lower_pos].second) break;
                    lower = array[lower_pos].first;
                    ++lower_pos;
                }
                upper = std::min(upper, array[lower_pos].first - 1);
                *index = lower_pos;
                *relative_lower = lower_pos > 0 ? lower - array[lower_pos - 1].first : lower;
                *relative_upper = lower_pos > 0 ? upper - array[lower_pos - 1].first : upper;
                return true;
            }
            *index = lower_pos;
            *relative_lower = lower % array[0].first;
            *relative_upper = upper % array[0].first;
            return true;

        } else {
            size_t left = 0, right = array.size() - 1;
            while (left < right) {
                size_t mid = (left + right) / 2;
                if (lower < array[mid].first) right = mid;
                else left = mid + 1;
            }

            if (upper >= array[left].first) {
                while (true) {
                    if (left >= array.size()) return false;
                    if (key <= array[left].second) break;
                    lower = array[left].first;
                    ++left;
                }
                upper = std::min(upper, array[left].first - 1);
            }


            *index = left;
            *relative_lower = left > 0 ? lower - array[left - 1].first : lower;
            *relative_upper = left > 0 ? upper - array[left - 1].first : upper;
            return true;
        }
    }

    bool AccumulatedNumEntriesArray::SearchNoError(uint64_t position, size_t *index, uint64_t *relative_position) {
        *index = position / array[0].first;
        *relative_position = position % array[0].first;
        return *index < array.size();

//        size_t left = 0, right = array.size() - 1;
//        while (left < right) {
//            size_t mid = (left + right) / 2;
//            if (position < array[mid].first) right = mid;
//            else left = mid + 1;
//        }
//        *index = left;
//        *relative_position = left > 0 ? position - array[left - 1].first : position;
//        return left < array.size();
    }

    uint64_t AccumulatedNumEntriesArray::NumEntries() const {
        return array.empty() ? 0 : array.back().first;
    }






    uint64_t ExtractInteger(const char* pos, size_t size) {
        char* temp = new char[size + 1];
        memcpy(temp, pos, size);
        temp[size] = '\0';
        uint64_t result = (uint64_t) atol(temp);
        delete[] temp;
        return result;
    }

//    bool SearchNumEntriesArray(const std::vector<uint64_t>& num_entries_array, const uint64_t pos,
//                                size_t* index, uint64_t* relative_pos) {
//        size_t left = 0, right = num_entries_array.size() - 1;
//        while (left < right) {
//            size_t mid = (left + right) / 2;
//            if (pos < num_entries_array[mid]) right = mid;
//            else left = mid + 1;
//        }
//        *index = left;
//        *relative_pos = left > 0 ? pos - num_entries_array[left - 1] : pos;
//        return left < num_entries_array.size();
//    }


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

    uint64_t SliceToInteger(const Slice& slice) {
        const char* data = slice.data();
        size_t size = slice.size();
        uint64_t num = 0;
        bool leading_zeros = true;

        for (int i = 0; i < size; ++i) {
            int temp = data[i];
            if (leading_zeros && temp == '0') continue;
            leading_zeros = false;
            num = (num << 3) + (num << 1) + temp - 48;
        }
        return num;
    }

    int compare(const Slice& slice, const string& string) {
        return memcmp((void*) slice.data(), string.c_str(), slice.size());
    }

    bool operator<(const Slice& slice, const string& string) {
        return memcmp((void*) slice.data(), string.c_str(), slice.size()) < 0;
    }
    bool operator>(const Slice& slice, const string& string) {
        return memcmp((void*) slice.data(), string.c_str(), slice.size()) > 0;
    }
    bool operator<=(const Slice& slice, const string& string) {
        return memcmp((void*) slice.data(), string.c_str(), slice.size()) <= 0;
    }
    bool operator>=(const Slice& slice, const string& string) {
        return memcmp((void*) slice.data(), string.c_str(), slice.size()) >= 0;
    }
}

































