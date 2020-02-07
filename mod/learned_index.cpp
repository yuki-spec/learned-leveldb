//
// Created by daiyi on 2020/02/02.
//

#include <cstdint>
#include <cassert>
#include <utility>
#include <cmath>
#include <iostream>
#include "util/mutexlock.h"
#include "learned_index.h"
#include "util.h"
#include "plr.h"
#include "db/version_set.h"

namespace adgMod {

    void LearnedIndexData::AddSegment(string&& x, double k, double b) {
        if (string_mode) {
            string_segments.push_back(Segment(x, k, b));
        } else {
            assert(false);
        }
    }

    std::pair<uint64_t, uint64_t> LearnedIndexData::GetPosition(const Slice& target_x) const {
        assert(string_segments.size() > 1);

        uint64_t target_int = SliceToInteger(target_x);
        if (target_x > max_key) return std::make_pair(size, size);
        if (target_x < min_key) return std::make_pair(size, size);

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

    uint64_t LearnedIndexData::MaxPosition() const {
        return size - 1;
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
        min_key = string_keys.front();
        max_key = string_keys.back();
        size = string_keys.size();

        for (auto& str: string_segments) {
            //printf("%s %f\n", str.first.c_str(), str.second);
        }

        learned.store(true);
        return;
    }

    void LearnedIndexData::Learn(void *arg) {
        Stats* instance = Stats::GetInstance();
        instance->StartTimer(8);

        VersionAndSelf* vas = reinterpret_cast<VersionAndSelf*>(arg);
        Version* version = vas->version;
        LearnedIndexData* self = vas->self;

        Version* current = adgMod::db->GetCurrentVersion();

        if (current == version){
            current->FillLevel(*adgMod::read_options, vas->level);
            self->filled = true;
            self->Learn();
            std::cout << "Learning complete" << std::endl;
        }
        adgMod::db->ReturnCurrentVersion(current);
        delete vas;

        instance->PauseTimer(8, true);
    }

    void LearnedIndexData::FileLearn(void *arg) {
        Stats* instance = Stats::GetInstance();
        instance->StartTimer(8);

        MetaAndSelf* mas = reinterpret_cast<MetaAndSelf*>(arg);
        LearnedIndexData* self = mas->self;

        Version* current = adgMod::db->GetCurrentVersion();
        if (self->FillData(current, mas->meta)) {
            self->Learn();
        }
        adgMod::db->ReturnCurrentVersion(current);
        delete mas;

        instance->PauseTimer(8, true);
    }

    bool LearnedIndexData::Learned(Version* version, int level) {
        if (learned_not_atomic) return true;
        else if (learned.load()) {
            learned_not_atomic = true;
            return true;
        } else {
            if (!learning_not_atomic && !learning.load()) {
                learning_not_atomic = true;
                env->Schedule(&LearnedIndexData::Learn, new VersionAndSelf{version, this, level});
            }
            return false;
        }
    }

    bool LearnedIndexData::Learned(FileMetaData *meta) {
        if (learned_not_atomic) return true;
        else if (learned.load()) {
            learned_not_atomic = true;
            return true;
        } else {
            if (!learning_not_atomic && !learning.load()) {
                learning_not_atomic = true;
                env->Schedule(&LearnedIndexData::FileLearn, new MetaAndSelf{meta, this});
            }
            return false;
        }
    }

    bool LearnedIndexData::FillData(Version *version, FileMetaData *meta) {
        if (filled) return true;

        if (version->FillData(*adgMod::read_options, meta, this)) {
            filled = true;
            return true;
        }
        return false;
    }








    bool FileLearnedIndexData::FillData(Version *version, FileMetaData *meta) {
        {
            leveldb::MutexLock l(&mutex);
            if (file_learned_index_data.size() <= meta->number)
                file_learned_index_data.resize(meta->number + 1, nullptr);
            if (file_learned_index_data[meta->number] == nullptr)
                file_learned_index_data[meta->number] = new LearnedIndexData();
        }
        return file_learned_index_data[meta->number]->FillData(version ,meta);
    }

    std::vector<std::string>& FileLearnedIndexData::GetData(FileMetaData *meta) {
        return file_learned_index_data[meta->number]->string_keys;
    }

    bool FileLearnedIndexData::Learned(Version* version, FileMetaData* meta) {
        leveldb::MutexLock l(&mutex);
        if (file_learned_index_data.size() <= meta->number)
            file_learned_index_data.resize(meta->number + 1, nullptr);
        if (file_learned_index_data[meta->number] == nullptr)
            file_learned_index_data[meta->number] = new LearnedIndexData();
        return file_learned_index_data[meta->number]->Learned(meta);
    }

    AccumulatedNumEntriesArray* FileLearnedIndexData::GetAccumulatedArray(int file_num) {
        return &file_learned_index_data[file_num]->num_entries_accumulated;
    }

    std::pair<uint64_t, uint64_t> FileLearnedIndexData::GetPosition(const Slice &key, int file_num) {
        return file_learned_index_data[file_num]->GetPosition(key);
    }










    void AccumulatedNumEntriesArray::Add(uint64_t num_entries, string &&key) {
        array.emplace_back(num_entries, key);
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

}
