#include <cassert>
#include <chrono>
#include <iostream>
#include "leveldb/db.h"
#include "leveldb/comparator.h"
#include "util.h"
#include "stats.h"
#include "learned_index.h"
#include <cstring>
#include "cxxopts.hpp"
#include <unistd.h>
#include <fstream>
#include "../db/version_set.h"

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


void PutAndPrefetch(int lower, int higher, vector<string>& keys) {
    adgMod::Stats* instance = adgMod::Stats::GetInstance();

    Status status;

    instance->StartTimer(9);
    for (int i = lower; i < higher; ++i) {
        string value = generate_value(0);
        status = db->Put(write_options, keys[i], value);
        assert(status.ok() && "File Put Error");
    }
    instance->PauseTimer(9, true);

    //cout << "Put Complete" << endl;


    instance->StartTimer(10);
    for (int i = lower; i < higher; ++i) {
        string value;
        status = db->Get(read_options, keys[i], &value);
        //cout << "Get " << keys[i] << " Done" << endl;
        assert(status.ok() && "File Get Error");
    }
    instance->PauseTimer(10, true);

    //cout << "Prefetch Complete" << endl;
};

enum LoadType {
    Ordered = 0,
    Reversed = 1,
    ReversedChunk = 2,
    Random = 3,
    RandomChunk = 4
};

int main(int argc, char *argv[]) {
    int num_operations, num_iteration, num_mix;
    float test_num_segments_base;
    float num_pair_step;
    string db_location, profiler_out, input_filename, distribution_filename;
    bool print_single_timing, print_file_info, evict, unlimit_fd, use_distribution = false, pause;
    bool change_level_load, change_file_load, change_level_learning, change_file_learning;
    int load_type;
    string db_location_copy;

    cxxopts::Options commandline_options("leveldb read test", "Testing leveldb read performance.");
    commandline_options.add_options()
            ("n,get_number", "the number of gets (to be multiplied by 1024)", cxxopts::value<int>(num_operations)->default_value("1024"))
            ("s,step", "the step of the loop of the size of db", cxxopts::value<float>(num_pair_step)->default_value("1"))
            ("i,iteration", "the number of iterations of a same size", cxxopts::value<int>(num_iteration)->default_value("1"))
            ("m,modification", "if set, run our modified version", cxxopts::value<int>(adgMod::MOD)->default_value("0"))
            ("h,help", "print help message", cxxopts::value<bool>()->default_value("false"))
            ("d,directory", "the directory of db", cxxopts::value<string>(db_location)->default_value("/mnt/ssd/testdb"))
            ("k,key_size", "the size of key", cxxopts::value<int>(adgMod::key_size)->default_value("8"))
            ("v,value_size", "the size of value", cxxopts::value<int>(adgMod::value_size)->default_value("8"))
            ("single_timing", "print the time of every single get", cxxopts::value<bool>(print_single_timing)->default_value("false"))
            ("file_info", "print the file structure info", cxxopts::value<bool>(print_file_info)->default_value("false"))
            ("test_num_segments", "test: number of segments per level", cxxopts::value<float>(test_num_segments_base)->default_value("1"))
            ("string_mode", "test: use string or int in model", cxxopts::value<bool>(adgMod::string_mode)->default_value("false"))
            ("e,model_error", "error in modesl", cxxopts::value<uint32_t>(adgMod::model_error)->default_value("8"))
            ("f,input_file", "the filename of input file", cxxopts::value<string>(input_filename)->default_value(""))
            ("multiple", "test: use larger keys", cxxopts::value<uint64_t>(adgMod::key_multiple)->default_value("1"))
            ("w,write", "writedb", cxxopts::value<bool>(fresh_write)->default_value("false"))
            ("c,uncache", "evict cache", cxxopts::value<bool>(evict)->default_value("false"))
            ("u,unlimit_fd", "unlimit fd", cxxopts::value<bool>(unlimit_fd)->default_value("false"))
            ("x,dummy", "dummy option")
            ("l,load_type", "load type", cxxopts::value<int>(load_type)->default_value("0"))
            ("filter", "use filter", cxxopts::value<bool>(adgMod::use_filter)->default_value("false"))
            ("mix", "mix read and write", cxxopts::value<int>(num_mix)->default_value(to_string(std::numeric_limits<int>::max())))
            ("distribution", "operation distribution", cxxopts::value<string>(distribution_filename)->default_value(""))
            ("change_level_load", "load level model", cxxopts::value<bool>(change_level_load)->default_value("false"))
            ("change_file_load", "enable level learning", cxxopts::value<bool>(change_file_load)->default_value("false"))
            ("change_level_learning", "load file model", cxxopts::value<bool>(change_level_learning)->default_value("false"))
            ("change_file_learning", "enable file learning", cxxopts::value<bool>(change_file_learning)->default_value("false"))
            ("p,pause", "pause between operation", cxxopts::value<bool>(pause)->default_value("false"));
    auto result = commandline_options.parse(argc, argv);
    if (result.count("help")) {
        printf("%s", commandline_options.help().c_str());
        exit(0);
    }

    std::default_random_engine e1, e2;
    num_operations *= num_pairs_base;
    db_location_copy = db_location;

    adgMod::fd_limit = unlimit_fd ? 1024 * 1024 : 1024;
    adgMod::restart_read = true;
    adgMod::level_learning_enabled ^= change_level_learning;
    adgMod::file_learning_enabled ^= change_file_learning;
    adgMod::load_level_model ^= change_level_load;
    adgMod::load_file_model ^= change_file_load;

    vector<string> keys;
    vector<uint64_t> distribution;
    //keys.reserve(100000000000 / adgMod::value_size);
    if (!input_filename.empty()) {
        ifstream input(input_filename);
        string key;
        while (input >> key) {
            string the_key = generate_key(key);
            keys.push_back(std::move(the_key));
        }
        adgMod::key_size = (int) keys.front().size();
    }

    if (!distribution_filename.empty()) {
        use_distribution = true;
        ifstream input(distribution_filename);
        uint64_t index;
        while (input >> index) {
            distribution.push_back(index);
        }
    }


    adgMod::Stats* instance = adgMod::Stats::GetInstance();
    vector<vector<size_t>> times(20);
    string values(1024 * 1024, '0');
    
    for (size_t iteration = 0; iteration < num_iteration; ++iteration) {
        system("sudo fstrim -a -v");
        db_location = db_location_copy;
        std::uniform_int_distribution<uint64_t > uniform_dist_file(0, (uint64_t) keys.size() - 1);
        std::uniform_int_distribution<uint64_t > uniform_dist_value(0, (uint64_t) values.size() - adgMod::value_size - 1);

        DB* db;
        Options options;
        ReadOptions& read_options = adgMod::read_options;
        WriteOptions& write_options = adgMod::write_options;
        Status status;

        options.create_if_missing = true;
        //options.comparator = new NumericalComparator;
        adgMod::block_restart_interval = options.block_restart_interval = adgMod::MOD == 6 || adgMod::MOD == 7 ? 1 : adgMod::block_restart_interval;
        //read_options.fill_cache = true;
        write_options.sync = false;
        instance->ResetAll(true);


        if (fresh_write && iteration == 0) {
            string command = "rm -rf " + db_location;
            system(command.c_str());
            system("sync; echo 3 | sudo tee /proc/sys/vm/drop_caches");
            cout << "delete and trim complete" << endl;

            status = DB::Open(options, db_location, &db);
            assert(status.ok() && "Open Error");


            instance->StartTimer(9);
            int cut_size = keys.size() / 100000;
            std::vector<std::pair<int, int>> chunks;
            switch (load_type) {
                case Ordered: {
                    for (int cut = 0; cut < cut_size; ++cut) {
                        chunks.emplace_back(keys.size() * cut / cut_size, keys.size() * (cut + 1) / cut_size);
                    }
                    break;
                }
                case ReversedChunk: {
                    for (int cut = cut_size - 1; cut >= 0; --cut) {
                        chunks.emplace_back(keys.size() * cut / cut_size, keys.size() * (cut + 1) / cut_size);
                    }
                    break;
                }
                case Random: {
                    std::random_shuffle(keys.begin(), keys.end());
                    for (int cut = 0; cut < cut_size; ++cut) {
                        chunks.emplace_back(keys.size() * cut / cut_size, keys.size() * (cut + 1) / cut_size);
                    }
                    break;
                }
                case RandomChunk: {
                    for (int cut = 0; cut < cut_size; ++cut) {
                        chunks.emplace_back(keys.size() * cut / cut_size, keys.size() * (cut + 1) / cut_size);
                    }
                    std::random_shuffle(chunks.begin(), chunks.end());
                    break;
                }
                default: assert(false && "Unsupported load type.");
            }

            for (int cut = 0; cut < chunks.size(); ++cut) {
                for (int i = chunks[cut].first; i < chunks[cut].second; ++i) {


                    //cout << keys[i] << endl;

                    status = db->Put(write_options, keys[i], {values.data() + uniform_dist_value(e2), (uint64_t) adgMod::value_size});

                    assert(status.ok() && "File Put Error");
                }
            }
            adgMod::db->vlog->Sync();
            instance->PauseTimer(9, true);
            cout << "Put Complete" << endl;

            keys.clear();

            if (print_file_info && iteration == 0) db->PrintFileInfo();
            adgMod::db->WaitForBackground();
            delete db;
            status = DB::Open(options, db_location, &db);
            adgMod::db->WaitForBackground();
            if (adgMod::MOD == 6 || adgMod::MOD == 7) {
                Version* current = adgMod::db->versions_->current();
                for (int i = 1; i < config::kNumLevels; ++i) {
                    LearnedIndexData::Learn(new VersionAndSelf{current, adgMod::db->version_count, current->learned_index_data_[i].get(), i});
                }
                current->FileLearn();
            }
            cout << "Shutting down" << endl;
            adgMod::db->WaitForBackground();
            delete db;

            //keys.reserve(100000000000 / adgMod::value_size);
            if (!input_filename.empty()) {
                ifstream input(input_filename);
                string key;
                while (input >> key) {
                    string the_key = generate_key(key);
                    keys.push_back(std::move(the_key));
                }
                adgMod::key_size = (int) keys.front().size();
            }
            fresh_write = false;
        }



        if (num_mix < num_operations) {
            string db_location_mix = db_location + "_mix";
            string remove_command = "rm -rf " + db_location_mix;
            string copy_command = "cp -r " + db_location + " " + db_location_mix;
            system(remove_command.c_str());
            system(copy_command.c_str());
            db_location = db_location_mix;
        }




        if (evict) system("sync; echo 3 | sudo tee /proc/sys/vm/drop_caches");

        cout << "Starting up" << endl;
        status = DB::Open(options, db_location, &db);
        adgMod::db->WaitForBackground();
        assert(status.ok() && "Open Error");
//            for (int s = 12; s < 20; ++s) {
//                instance->ResetTimer(s);
//            }

//        if (adgMod::MOD == 6 || adgMod::MOD == 7) {
//            for (int i = 1; i < config::kNumLevels; ++i) {
//                Version* current = adgMod::db->versions_->current();
//                LearnedIndexData::Learn(new VersionAndSelf{current, adgMod::db->version_count, current->learned_index_data_[i].get(), i});
//            }
//        }
//        cout << "Shutting down" << endl;
//        adgMod::db->WaitForBackground();
//        delete db;
//        return 0;


        for (int i = 0; i < num_operations; ++i) {
            instance->StartTimer(10);

            uint64_t index = use_distribution ? distribution[i] : uniform_dist_file(e1) % (keys.size() - 1);

            if (i != 0 && i % num_mix == 0) {
                status = db->Put(write_options, keys[index], {values.data() + uniform_dist_value(e2), (uint64_t) adgMod::value_size});
                assert(status.ok() && "Mix Put Error");
            } else {
                string value;
                const string& key = keys[index];
                instance->StartTimer(4);
                status = db->Get(read_options, key, &value);
                instance->PauseTimer(4);

                //cout << "Get " << key << " : " << value << endl;
                if (!status.ok()) {
                    cout << key << " Not Found" << endl;
                    //assert(status.ok() && "File Get Error");
                }
            }
            instance->PauseTimer(10, true);
            if (pause) usleep(100);
        }


        instance->ReportTime();



        for (int i = 0; i < 10; ++i) {
            printf("Counter %d: %d\n", i, instance->ReportCounter(i));
            instance->ResetCounter(i);
        }
        instance->ReportLevelStats();

        for (int s = 0; s < times.size(); ++s) {
            times[s].push_back(instance->ReportTime(s));
        }
        adgMod::db->WaitForBackground();
        sleep(10);
        instance->ReportTimeSeries(7);
        instance->ReportTimeSeries(8);
        instance->ReportTimeSeries(9);
        instance->ReportTimeSeries(10);
        instance->ReportTimeSeries(11);

        delete db;
    }


    for (int s = 0; s < times.size(); ++s) {
        vector<uint64_t>& time = times[s];
        vector<double> diff(time.size());
        if (time.empty()) continue;

        double sum = std::accumulate(time.begin(), time.end(), 0.0);
        double mean = sum / time.size();
        std::transform(time.begin(), time.end(), diff.begin(), [mean] (double x) { return x - mean; });
        double stdev = std::sqrt(std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0) / time.size());

        printf("Timer %d MEAN: %lu, STDDEV: %f\n", s, (uint64_t) mean, stdev);
    }
}