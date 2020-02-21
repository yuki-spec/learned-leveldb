//
// Created by daiyi on 2020/02/02.
//

#include "util.h"
#include "learned_index.h"

using std::to_string;

namespace adgMod {

    int MOD = 5;
    bool string_mode = true;
    uint64_t key_multiple = 1;
    uint32_t model_error = 10;
    int block_restart_interval = 16;
    uint32_t test_num_level_segments = 100;
    uint32_t test_num_file_segments = 100;
    int key_size;
    int value_size;
    leveldb::Env* env;
    leveldb::DBImpl* db;
    leveldb::ReadOptions read_options;
    leveldb::WriteOptions write_options;
    FileLearnedIndexData* file_data = nullptr;

    int file_allowed_seek = 1;
    int level_allowed_seek = file_allowed_seek * 1000;
    float reference_frequency = 2.6;

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

    uint64_t get_time_difference(timespec start, timespec stop) {
        return (stop.tv_sec - start.tv_sec) * 1000000000 + stop.tv_nsec - start.tv_nsec;
    }

}