#include <cassert>
#include <chrono>
#include <iostream>
#include "leveldb/db.h"
#include "leveldb/comparator.h"
#include "util.h"
#include "stats.h"
#include <cstring>
#include "cxxopts.hpp"
#include <gperftools/profiler.h>
#include <unistd.h>
#include <fstream>

using namespace leveldb;
using namespace adgMod;
using std::string;
using std::cout;
using std::endl;
using std::to_string;
using std::vector;
using std::map;
using std::ifstream;
using std::string;

int num_pairs_base = 1024;

class NumericalComparator : public Comparator {
public:
    NumericalComparator() = default;
    virtual const char* Name() const {return "adgMod:NumericalComparator";}
    virtual int Compare(const Slice& a, const Slice& b) const {
        uint64_t ia = adgMod::ExtractInteger(a.data(), a.size());
        uint64_t ib = adgMod::ExtractInteger(b.data(), b.size());
        if (ia < ib) return -1;
        else if (ia == ib) return 0;
        else return 1;
    }
    virtual void FindShortestSeparator(std::string* start, const Slice& limit) const { return; };
    virtual void FindShortSuccessor(std::string* key) const { return; };
};


int main(int argc, char *argv[]) {
    int num_gets, num_iteration;
    float test_num_segments_base;
    float num_pair_lower, num_pair_upper, num_pair_step;
    string db_location, profiler_out, input_filename;
    bool print_single_timing, print_file_info;

    cxxopts::Options commandline_options("leveldb read test", "Testing leveldb read performance.");
    commandline_options.add_options()
            ("n,get_number", "the number of gets (to be multiplied by 1024)", cxxopts::value<int>(num_gets)->default_value("128"))
            ("l,lower_bound", "the lower bound of the loop of the size of db", cxxopts::value<float>(num_pair_lower)->default_value("10"))
            ("u,upper_bound", "the upper bound of the loop of the size of db", cxxopts::value<float>(num_pair_upper)->default_value("10"))
            ("s,step", "the step of the loop of the size of db", cxxopts::value<float>(num_pair_step)->default_value("1"))
            ("i,iteration", "the number of iterations of a same size", cxxopts::value<int>(num_iteration)->default_value("1"))
            ("m,modification", "if set, run our modified version", cxxopts::value<int>(adgMod::MOD)->default_value("0"))
            ("h,help", "print help message", cxxopts::value<bool>()->default_value("false"))
            ("d,directory", "the directory of db", cxxopts::value<string>(db_location)->default_value("/tmp/testdb"))
            ("k,key_size", "the size of key", cxxopts::value<int>(adgMod::key_size)->default_value("8"))
            ("v,value_size", "the size of value", cxxopts::value<int>(adgMod::value_size)->default_value("120"))
            ("single_timing", "print the time of every single get", cxxopts::value<bool>(print_single_timing)->default_value("false"))
            ("p,profile_out", "the file for profiler output", cxxopts::value<string>(profiler_out)->default_value("/tmp/profiler.out"))
            ("file_info", "print the file structure info", cxxopts::value<bool>(print_file_info)->default_value("false"))
            ("test_num_segments", "test: number of segments per level", cxxopts::value<float>(test_num_segments_base)->default_value("1"))
            ("string_mode", "test: use string or int in model", cxxopts::value<bool>(adgMod::string_mode)->default_value("false"))
            ("e,model_error", "error in modesl", cxxopts::value<uint32_t>(adgMod::model_error)->default_value("10"))
            ("f,input_file", "the filename of input file", cxxopts::value<string>(input_filename)->default_value(""))
            ("multiple", "test: use larger keys", cxxopts::value<uint64_t>(adgMod::key_multiple)->default_value("1"));
    auto result = commandline_options.parse(argc, argv);
    if (result.count("help")) {
        printf("%s", commandline_options.help().c_str());
        exit(0);
    }

    std::default_random_engine e1;
    num_gets *= 1024;


    vector<float> num_pairs;
    for (float i = num_pair_lower; i <= num_pair_upper; i+= num_pair_step) {
        num_pairs.push_back((float) pow(2, i));
    }

    vector<string> keys;
    if (!input_filename.empty()) {
        ifstream input(input_filename);
        string key;
        while (input >> key) {
            keys.push_back(std::move(key));
        }
        adgMod::key_size = (int) keys.front().size();
        input.close();
    }

    adgMod::Stats* instance = adgMod::Stats::GetInstance();
    for (size_t outer = 0; outer < num_pairs.size(); ++outer) {
        vector<size_t> time_sums(11, 0);
        adgMod::test_num_level_segments =  (uint32_t) floor(num_pairs[outer] *  test_num_segments_base);
        for (size_t iteration = 0; iteration < num_iteration; ++iteration) {
            string command = "rm -rf " + db_location;
            system(command.c_str());

            DB* db;
            Options options;
            ReadOptions read_options;
            WriteOptions write_options;

            options.create_if_missing = true;
            //options.comparator = new NumericalComparator;
            adgMod::block_restart_interval = options.block_restart_interval =
                adgMod::MOD ? 1 : adgMod::block_restart_interval;
            read_options.fill_cache = false;
            write_options.sync = false;

            Status status = DB::Open(options, db_location, &db);
            assert(status.ok() && "Open Error");

            instance->ResetAll(true);
            instance->StartTimer(9);
            if (input_filename.empty()) {
                for(uint64_t i = 0; i < num_pairs[outer] * num_pairs_base; ++i) {
                    //string key = generate_key(i);
                    //string value = generate_value(i);
                    status = db->Put(write_options, generate_key(i * adgMod::key_multiple), generate_value(i));
                    assert(status.ok() && "Put Error");
                }
            } else {
                for (int i = 0; i < keys.size(); ++i) {
                    string value = generate_value(0);
                    status = db->Put(write_options, keys[i], value);
                    assert(status.ok() && "File Put Error");
                }
            }
            instance->PauseTimer(9, true);

            cout << "Put Complete" << endl;
            sleep(1);
            instance->StartTimer(8);
            if (MOD > 0) db->Learn(read_options);
            instance->PauseTimer(8, true);
            cout << "Learn Complete" << endl;
            if (print_file_info && iteration == 0) db->PrintFileInfo();

            std::uniform_int_distribution<uint64_t > uniform_dist(0, (uint64_t) floor(num_pairs[outer] * num_pairs_base) - 1);
            std::uniform_int_distribution<uint64_t > uniform_dist_file(0, (uint64_t) keys.size() - 1);

            instance->StartTimer(10);
            if (input_filename.empty()) {
                for (uint64_t i = 0; i < num_pairs[outer] * num_pairs_base; ++i) {
                    string value;
                    status = db->Get(read_options, generate_key(i * adgMod::key_multiple), &value);
                    //cout << "Get " << i << " Done" << endl;
                    assert(status.ok() && "Get Error");
                }
            } else {
                for (int i = 0; i < keys.size(); ++i) {
                    string value;
                    status = db->Get(read_options, keys[i], &value);
                    //cout << "Get " << keys[i] << " Done" << endl;
                    assert(status.ok() && "File Get Error");
                }
            }
            instance->PauseTimer(10, true);

#ifdef PROFILER
            ProfilerStart(profiler_out.c_str());
#endif
            for (int s = 7; s <= 9; ++s)
                time_sums[s] += instance->ReportTime(s);
            instance->ResetAll();

            instance->StartTimer(10);
            if (input_filename.empty()) {
                for (uint64_t i = 0; i < num_gets; ++i) {
                    string value;
                    string key = generate_key(uniform_dist(e1) * adgMod::key_multiple);
                    uint64_t start_time = instance->ReportTime(4);
                    instance->StartTimer(4);
                    status = db->Get(read_options, key, &value);
                    instance->PauseTimer(4);
                    assert(status.ok() && "Get Error");
                    if (print_single_timing) {
                        uint64_t time_elapse = instance->ReportTime(4) - start_time;
                        printf("*** %lu\n", time_elapse);
                    }
                }
            } else {
                for (int i = 0; i < num_gets; ++i) {
                    string value;
                    instance->StartTimer(4);
                    status = db->Get(read_options, keys[uniform_dist_file(e1)], &value);
                    instance->PauseTimer(4);
                    assert(status.ok() && "File Get Error");
                }
            }
            instance->PauseTimer(10, true);

#ifdef PROFILER
            ProfilerStop();
#endif

            cout << num_pairs[outer] << " " << instance->ReportTime(4) << " " << iteration << endl;
            instance->ReportTime();
            instance->ReportLevelStats();

            for (int s = 0; s < time_sums.size(); ++s) {
                time_sums[s] += instance->ReportTime(s);
            }
            delete db;
        }

        for (int s = 0; s < time_sums.size(); ++s) {
            printf("%d : Time Average for Timer %d : %lu\n", int(num_pairs[outer]), s, time_sums[s] / num_iteration);
        }
        instance->ReportTimeSeries(7);
        instance->ReportTimeSeries(8);
        instance->ReportTimeSeries(9);
        instance->ReportTimeSeries(10);


        if (!input_filename.empty()) break;
    }
}