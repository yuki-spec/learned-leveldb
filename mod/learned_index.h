//
// Created by daiyi on 2020/02/02.
//

#ifndef LEVELDB_LEARNED_INDEX_H
#define LEVELDB_LEARNED_INDEX_H


#include <vector>
#include <cstring>
#include "util.h"
#include "leveldb/slice.h"
#include "leveldb/db.h"
#include <atomic>



using std::string;
using leveldb::Slice;
using leveldb::Version;

namespace adgMod {




    class Segment {
    public:
        Segment(string _x, double _k, double _b) : x(_x), k(_k), b(_b) {}
        string x;
        double k;
        double b;
    };


    class LearnedIndexData {
    private:
        bool string_mode;
        double error;
        std::atomic<bool> learned;
        std::atomic<bool> learning;
        bool learned_not_atomic;
        bool learning_not_atomic;
        Version* current;

        std::vector<Segment> string_segments;

        class VersionAndSelf {
        public:
            Version* version;
            LearnedIndexData* self;
        };
    public:
        std::vector<std::string> string_keys;

        LearnedIndexData() : string_mode(adgMod::string_mode), error(adgMod::model_error), learned(false), learning(false), learned_not_atomic(false), learning_not_atomic(false) {};
        LearnedIndexData(const LearnedIndexData& other) : string_mode(adgMod::string_mode), error(adgMod::model_error), learned(false), learning(false), learned_not_atomic(false), learning_not_atomic(false) {};
        void AddSegment(string&& x, double k, double b);
        std::pair<uint64_t, uint64_t> GetPosition(const Slice& key) const;
        uint64_t MaxPosition() const;
        double GetError() const;
        void Learn();
        bool Learned();
        static void Learn(void* self);
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

}

#endif //LEVELDB_LEARNED_INDEX_H
