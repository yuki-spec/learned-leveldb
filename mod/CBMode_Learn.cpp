#include <util/mutexlock.h>
#include "CBMode_Learn.h"




CBModel_Learn::CBModel_Learn() : negative_lookups_time(2), positive_lookups_time(2) {};

void CBModel_Learn::AddLookupData(int level, bool positive, bool model, uint64_t value) {
    leveldb::MutexLock guard(&lookup_mutex);
    std::vector<Counter>& target = positive ? positive_lookups_time : negative_lookups_time;
    target[model].Increment(level, value);
}

void CBModel_Learn::AddFileData(int level, uint64_t num_negative, uint64_t num_positive, uint64_t size) {
    leveldb::MutexLock guard(&file_mutex);
    num_negative_lookups_file.Increment(level, num_negative);
    num_positive_lookups_file.Increment(level, num_positive);
    file_sizes.Increment(level, size);
}

bool CBModel_Learn::CalculateCB(int level, uint64_t file_size) {
    //return true;
    int num_pos[2] = {0, 0}, num_neg[2] = {0, 0}, num_files = 0;
    uint64_t time_pos[2] = {0, 0}, time_neg[2] = {0, 0}, num_neg_lookups_file, num_pos_lookups_file, size_sum;
    {
        leveldb::MutexLock guard(&lookup_mutex);
        for (int i = 0; i < 2; ++i) {
            num_pos[i] = positive_lookups_time[i].nums[level];
            num_neg[i] = negative_lookups_time[i].nums[level];
            time_pos[i] = positive_lookups_time[i].counts[level];
            time_neg[i] = negative_lookups_time[i].counts[level];
        }
    }
    {
        leveldb::MutexLock guard(&file_mutex);
        num_files = num_negative_lookups_file.nums[level];
        num_neg_lookups_file = num_negative_lookups_file.counts[level];
        num_pos_lookups_file = num_positive_lookups_file.counts[level];
    }

    if (num_files < file_average_limit[level]) return true;
    double average_pos_lookups = (double) num_pos_lookups_file / num_files;
    double average_neg_lookups = (double) num_neg_lookups_file / num_files;
    double average_pos_time[2] = {0, 0}, average_neg_time[2] = {0, 0};

    for (int i = 0; i < 2; ++i) {
        if (num_pos[i] + num_neg[i] < lookup_average_limit) return true;

        if (num_pos[i] < 100) {
            average_pos_lookups = 0;
            average_pos_time[i] = 0;
        } else {
            average_pos_time[i] = (double) time_pos[i] / num_pos[i];
        }

        if (num_neg[i] < 100) {
            average_neg_lookups = 0;
            average_neg_time[i] = 0;
        } else {
            average_neg_time[i] = (double) time_neg[i] / num_neg[i];
        }
    }

    double pos_gain = (average_pos_time[0] - average_pos_time[1]) * average_pos_lookups;
    double neg_gain = (average_neg_time[0] - average_neg_time[1]) * average_neg_lookups;

//    fprintf(stdout, "%f %lu %d %f %f %f %f %f %f\n", pos_gain + neg_gain, file_size * const_size_to_cost, level,
//             average_pos_time[0], average_pos_time[1], average_pos_lookups, average_neg_time[0], average_neg_time[1], average_neg_lookups);
    return pos_gain + neg_gain > file_size * const_size_to_cost;
}




























