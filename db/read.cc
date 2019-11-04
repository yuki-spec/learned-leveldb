#include <cassert>
#include <chrono>
#include <iostream>
#include "leveldb/db.h"
#include <cstring>
#include "cxxopts.hpp"
#include <gperftools/profiler.h>

using namespace std::chrono;
using namespace leveldb;
using std::string;
using std::cout;
using std::endl;
using std::to_string;
using std::vector;
using std::map;

using std::string;

int key_size;
int value_size;
int num_pairs_base = 1024;

string generate_key(int key) {
    string key_string = to_string(key);
    string result = string(key_size - key_string.length(), '0') + key_string;
    return std::move(result);
}

string generate_value(int value) {
    string value_string = to_string(value);
    string result = string(value_size - value_string.length(), '0') + value_string;
    return std::move(result);
}


int main(int argc, char *argv[]) {
    int num_gets, num_iteration;
    float num_pair_lower, num_pair_upper, num_pair_step;
    string db_location, profiler_out;
    bool print_single_timing, print_file_info;

    cxxopts::Options commandline_options("leveldb read test", "Testing leveldb read performance.");
    commandline_options.add_options()
            ("n,get_number", "the number of gets (to be multiplied by 1024)", cxxopts::value<int>(num_gets)->default_value("128"))
            ("l,lower_bound", "the lower bound of the loop of the size of db", cxxopts::value<float>(num_pair_lower)->default_value("10"))
            ("u,upper_bound", "the upper bound of the loop of the size of db", cxxopts::value<float>(num_pair_upper)->default_value("10"))
            ("s,step", "the step of the loop of the size of db", cxxopts::value<float>(num_pair_step)->default_value("1"))
            ("i,iteration", "the number of iterations of a same size", cxxopts::value<int>(num_iteration)->default_value("1"))
            ("m,modification", "if set, run our modified version", cxxopts::value<bool>(adgMod::MOD)->default_value("false"))
            ("h,help", "print help message", cxxopts::value<bool>()->default_value("false"))
            ("d,directory", "the directory of db", cxxopts::value<string>(db_location)->default_value("/tmp/testdb"))
            ("k,key_size", "the size of key", cxxopts::value<int>(key_size)->default_value("8"))
            ("v,value_size", "the size of value", cxxopts::value<int>(value_size)->default_value("120"))
            ("single_timing", "print the time of every single get", cxxopts::value<bool>(print_single_timing)->default_value("false"))
            ("p,profile_out", "the file for profiler output", cxxopts::value<string>(profiler_out)->default_value("/tmp/profiler.out"))
            ("file_info", "print the file structure info", cxxopts::value<bool>(print_file_info)->default_value("false"));
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

    for (size_t outer = 0; outer < num_pairs.size(); ++outer) {
        vector<size_t> time_sums(9, 0);
        
        for (size_t iteration = 0; iteration < num_iteration; ++iteration) {
            DB* db;
            Options options;
            ReadOptions read_options;
            WriteOptions write_options;

            options.create_if_missing = true;
            options.paranoid_checks = true;
            read_options.fill_cache = false;
            write_options.sync = false;

            Status status = DB::Open(options, db_location, &db);
            assert(status.ok() && "Open Error");

            for(int i = 0; i < num_pairs[outer] * num_pairs_base; ++i) {
                string key = generate_key(i);
                string value = generate_value(i);
                status = db->Put(write_options, key, value);
                assert(status.ok() && "Put Error");
            }
            
            std::uniform_int_distribution<int> uniform_dist(0, (int) floor(num_pairs[outer] * num_pairs_base) - 1);
            for (int i = 0; i < num_pairs[outer] * num_pairs_base; ++i) {
                string value;
                string key = generate_key(i);
                status = db->Get(read_options, key, &value);
                assert(status.ok() && "Get Error");
            }

            adgMod::Stats* instance = adgMod::Stats::GetInstance();
            instance->ResetAll();

#ifdef PROFILER
            ProfilerStart(profiler_out.c_str());
#endif
            
            for (int i = 0; i < num_gets; ++i) {
                string value;
                string key = generate_key(uniform_dist(e1));
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

#ifdef PROFILER
            ProfilerStop();
#endif

            cout << num_pairs[outer] << " " << instance->ReportTime(4) << " " << iteration << endl;
            instance->ReportTime();
            instance->ReportLevelStats();
            if (print_file_info && iteration == 0) db->PrintFileInfo();
            for (int s = 0; s < time_sums.size(); ++s) {
                time_sums[s] += instance->ReportTime(s);
            }
            instance->ResetAll();
            delete db;
            string command = "rm -rf " + db_location;
            system(command.c_str());
        }

        for (int s = 0; s < time_sums.size(); ++s) {
            printf("%d : Time Average for Timer %d : %lu\n", int(num_pairs[outer]), s, time_sums[s] / num_iteration);
        }
    }
}