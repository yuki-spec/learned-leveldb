//
// Created by daiyi on 2020/02/02.
//

#include <cstdint>
#include <cassert>
#include <utility>
#include <cmath>
#include <iostream>
#include <fstream>
#include "util/mutexlock.h"
#include "learned_index.h"
#include "util.h"
#include "plr.h"
#include "db/version_set.h"

namespace adgMod {

    std::pair<uint64_t, uint64_t> LearnedIndexData::GetPosition(const Slice& target_x) const {
        assert(string_segments.size() > 1);

        uint64_t target_int = SliceToInteger(target_x);
        if (target_int > max_key) return std::make_pair(size, size);
        if (target_int < min_key) return std::make_pair(size, size);

        uint32_t left = 0, right = (uint32_t) string_segments.size() - 1;
        while (left != right - 1) {
            uint32_t mid = (right + left) / 2;
            if (target_int < string_segments[mid].x) right = mid;
            else left = mid;
        }

        double result = target_int * string_segments[left].k + string_segments[left].b;
        uint64_t lower = result - error > 0 ? (uint64_t) std::floor(result - error) : 0;
        uint64_t upper = (uint64_t) std::ceil(result + error);
        if (lower >= size) return std::make_pair(size, size);
        upper = upper < size ? upper : size - 1;
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

    bool LearnedIndexData::Learn() {
        // FILL IN GAMMA (error)
        PLR plr = PLR(error);

        if (string_keys.empty()) assert(false);


        uint64_t temp = atoll(string_keys.back().c_str());
        min_key = atoll(string_keys.front().c_str());
        max_key = atoll(string_keys.back().c_str());
        size = string_keys.size();


        std::deque<struct segment> segs = plr.train(string_keys, !level);
        if (segs.empty()) return false;

        while (!segs.empty()) {
            segment& s = segs.front();
            string_segments.emplace_back(s.start, s.slope, s.intercept);
            segs.pop_front();
        }
        string_segments.emplace_back(temp, 0, 0);


        for (auto& str: string_segments) {
            //printf("%s %f\n", str.first.c_str(), str.second);
        }

        learned.store(true);
        return true;
    }

    void LearnedIndexData::Learn(void *arg) {
        Stats* instance = Stats::GetInstance();
        instance->StartTimer(8);

        VersionAndSelf* vas = reinterpret_cast<VersionAndSelf*>(arg);
        LearnedIndexData* self = vas->self;
        self->level = true;

        Version* c = db->GetCurrentVersion();
        if (db->version_count == vas->v_count){
            if (vas->version->FillLevel(adgMod::read_options, vas->level)) {
                //instance->ReportEventWithTime("Fill " + to_string(vas->level));
                self->filled = true;
                if (db->version_count == vas->v_count) {
                    if (env->compaction_awaiting.load() == 0 && self->Learn()) {
                        instance->ReportEventWithTime("L " + to_string(vas->level));
                    } else {
                        self->learning.store(false);
                    }
                }
            }
        }
        adgMod::db->ReturnCurrentVersion(c);


        instance->PauseTimer(8, true);
        instance->IncrementCounter(7, self->string_segments.size());
        instance->IncrementCounter(8, self->num_entries_accumulated.array.size());
        //self->WriteModel(vas->version->vset_->dbname_ + "/" + to_string(vas->level) + ".model");
        //self->string_segments.clear();
        //self->num_entries_accumulated.array.clear();
        delete vas;
    }

    void LearnedIndexData::FileLearn(void *arg) {
        Stats* instance = Stats::GetInstance();
        instance->StartTimer(11);

        MetaAndSelf* mas = reinterpret_cast<MetaAndSelf*>(arg);
        LearnedIndexData* self = mas->self;

        Version* c = db->GetCurrentVersion();
        if (db->version_count == mas->v_count && self->FillData(c, mas->meta)) {
            self->Learn();
            instance->ReportEventWithTime("FL " + to_string(mas->meta->number));
        } else {
            self->learning.store(false);
        }
        adgMod::db->ReturnCurrentVersion(c);
        delete mas;

        instance->PauseTimer(11, true);
    }

    bool LearnedIndexData::Learned() {
        if (learned_not_atomic) return true;
        else if (learned.load()) {
            learned_not_atomic = true;
            return true;
        } else return false;
    }

    bool LearnedIndexData::Learned(Version* version, int v_count, int level) {
        if (learned_not_atomic) return true;
        else if (learned.load()) {
            learned_not_atomic = true;
            return true;
        } else {
            if (level_learning_enabled && ++current_seek >= allowed_seek && !learning.exchange(true)) {
                env->ScheduleLearning(&LearnedIndexData::Learn, new VersionAndSelf{version, v_count, this, level}, 0);
            }
            return false;
        }
    }

    bool LearnedIndexData::Learned(Version* version, int v_count, FileMetaData *meta) {
        if (learned_not_atomic) return true;
        else if (learned.load()) {
            learned_not_atomic = true;
            return true;
        } else {
            if (file_learning_enabled && ++current_seek >= allowed_seek && !learning.exchange(true)) {
                env->ScheduleLearning(&LearnedIndexData::FileLearn, new MetaAndSelf{version, v_count, meta, this}, 0);
            }
            return false;
        }
    }

    bool LearnedIndexData::FillData(Version *version, FileMetaData *meta) {
        if (filled) return true;

        if (version->FillData(adgMod::read_options, meta, this)) {
            filled = true;
            return true;
        }
        return false;
    }

    void LearnedIndexData::WriteModel(const string &filename) {
        if (!learned.load()) return;

        std::ofstream output_file(filename);
        output_file.precision(15);
        output_file << adgMod::block_num_entries << " " << adgMod::block_size << " " << adgMod::entry_size << "\n";
        for (Segment& item: string_segments) {
            output_file << item.x << " " << item.k << " " << item.b << "\n";
        }
        output_file << "StartAcc" << " " << min_key << " " << max_key << " " << size << "\n";
        for (auto& pair: num_entries_accumulated.array) {
            output_file << pair.first << " " << pair.second << "\n";
        }
    }

    void LearnedIndexData::ReadModel(const string &filename) {
        std::ifstream input_file(filename);

        if (!input_file.good()) return;
        input_file >> adgMod::block_num_entries >> adgMod::block_size >> adgMod::entry_size;
        while (true) {
            string x;
            double k, b;
            input_file >> x;
            if (x == "StartAcc") break;
            input_file >> k >> b;
            string_segments.emplace_back(atoll(x.c_str()), k, b);
        }
        input_file >> min_key >> max_key >> size;
        while (true) {
            uint64_t first;
            string second;
            if (!(input_file >> first >> second)) break;
            num_entries_accumulated.Add(first, std::move(second));
        }

        learned.store(true);
    }






    LearnedIndexData* FileLearnedIndexData::GetModel(FileMetaData *meta) {
        leveldb::MutexLock l(&mutex);
        if (file_learned_index_data.size() <= meta->number)
            file_learned_index_data.resize(meta->number + 1, nullptr);
        if (file_learned_index_data[meta->number] == nullptr)
            file_learned_index_data[meta->number] = new LearnedIndexData(file_allowed_seek);
        return file_learned_index_data[meta->number];
    }

    bool FileLearnedIndexData::FillData(Version *version, FileMetaData *meta) {
        LearnedIndexData* model = GetModel(meta);
        return model->FillData(version, meta);
    }

    std::deque<std::string>& FileLearnedIndexData::GetData(FileMetaData *meta) {
        return file_learned_index_data[meta->number]->string_keys;
    }

    bool FileLearnedIndexData::Learned(Version* version, FileMetaData* meta) {
        LearnedIndexData* model = GetModel(meta);
        return model->Learned(version, db->version_count, meta);
    }

    AccumulatedNumEntriesArray* FileLearnedIndexData::GetAccumulatedArray(int file_num) {
        return &file_learned_index_data[file_num]->num_entries_accumulated;
    }

    std::pair<uint64_t, uint64_t> FileLearnedIndexData::GetPosition(const Slice &key, int file_num) {
        return file_learned_index_data[file_num]->GetPosition(key);
    }

    FileLearnedIndexData::~FileLearnedIndexData() {
        for (auto pointer: file_learned_index_data) {
            delete pointer;
        }
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
