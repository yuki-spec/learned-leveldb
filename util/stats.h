//
// Created by daiyi on 2019/09/30.
//

#ifndef LEVELDB_STATS_H
#define LEVELDB_STATS_H


#include <cstdint>
#include <map>
#include <vector>
#include <cstring>
#include "leveldb/slice.h"

using std::string;
using std::to_string;
using leveldb::Slice;

namespace adgMod {
    extern int MOD;
    extern bool string_mode;
    extern uint32_t model_error;
    extern int block_restart_interval;
    extern uint32_t test_num_level_segments;
    extern uint32_t test_num_file_segments;
    extern int key_size;
    extern int value_size;

    class Timer;
    class Stats {
    private:
        static Stats* singleton;
        Stats();

        std::vector<Timer> timers;
        std::vector<uint32_t> level_stats;
    public:
        static Stats* GetInstance();
        void StartTimer(uint32_t id);
        void PauseTimer(uint32_t id);
        void ResetTimer(uint32_t id);
        uint64_t ReportTime(uint32_t id);
        void ReportTime();

        void RecordLevel(int level);
        void ReportLevelStats();

        void ResetAll();
        ~Stats();
    };

    class LearnedIndexData {
    private:
        bool string_mode;
        float error;

        std::vector<std::pair<uint64_t, uint64_t>> segments;
        std::vector<std::pair<string, uint64_t>> string_segments;
    public:
        std::vector<string> string_keys;

        LearnedIndexData() : string_mode(adgMod::string_mode), error(adgMod::model_error) {};
        void AddSegment(string&& x, uint64_t y);
        std::pair<uint64_t, uint64_t> GetPosition(const Slice& key) const;
        uint64_t MaxPosition() const;
        float GetError() const;
        void Learn();
    };

    class AccumulatedNumEntriesArray {
    private:
        std::vector<std::pair<uint64_t, string>> array;
    public:
        AccumulatedNumEntriesArray() = default;
        void Add(uint64_t num_entries, string&& key);
        bool Search(const Slice& key, uint64_t lower, uint64_t upper, size_t* index, uint64_t* relative_lower, uint64_t* relative_upper);
        bool SearchNoError(uint64_t position, size_t* index, uint64_t* relative_position);
        uint64_t NumEntries() const;
    };

    uint64_t ExtractInteger(const char* pos, size_t size);
    //bool SearchNumEntriesArray(const std::vector<uint64_t>& num_entries_array, const uint64_t position, size_t* index, uint64_t* relative_position);
    string generate_key(uint64_t key);
    string generate_value(uint64_t value);
    uint64_t SliceToInteger(const Slice& slice);
    int compare(const Slice& slice, const string& string);
    bool operator<(const Slice& slice, const string& string);
    bool operator>(const Slice& slice, const string& string);
    bool operator<=(const Slice& slice, const string& string);
}


#endif //LEVELDB_STATS_H
