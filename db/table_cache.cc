// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <fcntl.h>
#include <table/filter_block.h>
#include "db/table_cache.h"
#include "db/filename.h"
#include "leveldb/env.h"
#include "leveldb/table.h"
#include "util/coding.h"
#include "mod/stats.h"
#include "table/block.h"
#include "db/version_set.h"


namespace leveldb {

struct TableAndFile {
  RandomAccessFile* file;
  Table* table;
};

static void DeleteEntry(const Slice& key, void* value) {
  TableAndFile* tf = reinterpret_cast<TableAndFile*>(value);
  delete tf->table;
  delete tf->file;
  delete tf;
}

static void UnrefEntry(void* arg1, void* arg2) {
  Cache* cache = reinterpret_cast<Cache*>(arg1);
  Cache::Handle* h = reinterpret_cast<Cache::Handle*>(arg2);
  cache->Release(h);
}

TableCache::TableCache(const std::string& dbname, const Options& options,
                       int entries)
    : env_(options.env),
      dbname_(dbname),
      options_(options),
      cache_(NewLRUCache(entries)),
      file_cache(NewLRUCache(adgMod::fd_limit)) {}

TableCache::~TableCache() { delete cache_; delete file_cache; }

Status TableCache::FindTable(uint64_t file_number, uint64_t file_size,
                             Cache::Handle** handle) {

  adgMod::Stats* instance = adgMod::Stats::GetInstance();


  Status s;
  char buf[sizeof(file_number)];
  EncodeFixed64(buf, file_number);
  Slice key(buf, sizeof(buf));
  *handle = cache_->Lookup(key);



  if (*handle == nullptr) {

    std::string fname = TableFileName(dbname_, file_number);
    RandomAccessFile* file = nullptr;
    Table* table = nullptr;
    s = env_->NewRandomAccessFile(fname, &file);
    if (!s.ok()) {
      std::string old_fname = SSTTableFileName(dbname_, file_number);
      if (env_->NewRandomAccessFile(old_fname, &file).ok()) {
        s = Status::OK();
      }
    }


    if (s.ok()) {
      s = Table::Open(options_, file, file_size, &table);
    }


    if (!s.ok()) {
      assert(table == nullptr);
      delete file;
      // We do not cache error results so that if the error is transient,
      // or somebody repairs the file, we recover automatically.
    } else {
      TableAndFile* tf = new TableAndFile;
      tf->file = file;
      tf->table = table;
      *handle = cache_->Insert(key, tf, 1, &DeleteEntry);
    }
  }
  return s;
}

Iterator* TableCache::NewIterator(const ReadOptions& options,
                                  uint64_t file_number, uint64_t file_size,
                                  Table** tableptr) {
  if (tableptr != nullptr) {
    *tableptr = nullptr;
  }

  Cache::Handle* handle = nullptr;
  Status s = FindTable(file_number, file_size, &handle);
  if (!s.ok()) {
    return NewErrorIterator(s);
  }

  Table* table = reinterpret_cast<TableAndFile*>(cache_->Value(handle))->table;
  Iterator* result = table->NewIterator(options);
  result->RegisterCleanup(&UnrefEntry, cache_, handle);
  if (tableptr != nullptr) {
    *tableptr = table;
  }
  return result;
}

Status TableCache::Get(const ReadOptions& options, uint64_t file_number,
                       uint64_t file_size, const Slice& k, void* arg,
                       void (*handle_result)(void*, const Slice&, const Slice&),
                       FileMetaData* meta, uint64_t lower, uint64_t upper, bool learned, Version* version) {
  Cache::Handle* handle = nullptr;
  adgMod::Stats* instance = adgMod::Stats::GetInstance();

  if ((adgMod::MOD == 6 || adgMod::MOD == 7) && meta != nullptr && learned) {
      LevelRead(options, file_number, file_size, k, arg, handle_result, meta, lower, upper, learned, version);
      return Status::OK();
  } else {
#ifdef INTERNAL_TIMER
      instance->StartTimer(1);
#endif
      Status s = FindTable(file_number, file_size, &handle);
#ifdef INTERNAL_TIMER
      instance->PauseTimer(1);
#endif
      if (s.ok()) {
          Table* t = reinterpret_cast<TableAndFile*>(cache_->Value(handle))->table;
          s = t->InternalGet(options, k, arg, handle_result, meta, lower, upper, learned, version);
          instance->StartTimer(1);
          cache_->Release(handle);
          instance->PauseTimer(1);
      }
      return s;
  }
}

void TableCache::Evict(uint64_t file_number) {
  char buf[sizeof(file_number)];
  EncodeFixed64(buf, file_number);
  cache_->Erase(Slice(buf, sizeof(buf)));
}

bool TableCache::FillData(const ReadOptions& options, FileMetaData *meta, adgMod::LearnedIndexData* data) {
  Cache::Handle* handle = nullptr;
  Status status = FindTable(meta->number, meta->file_size, &handle);

  if (!status.ok()) return false;

  Table* table = reinterpret_cast<TableAndFile*>(cache_->Value(handle))->table;
  table->FillData(options, data);
  cache_->Release(handle);
  return true;
}










class FilterAndFile {
public:
    RandomAccessFile* file;
    FilterBlockReader* filter;

    FilterAndFile() : file(nullptr), filter(nullptr) {}
    ~FilterAndFile() {
        delete file;
        delete filter;
    }
};

static void DeleteFilterAndFile(const Slice& key, void* value) {
    auto* filter_and_file = reinterpret_cast<FilterAndFile*>(value);
    delete filter_and_file;
}

void TableCache::LevelRead(const ReadOptions &options, uint64_t file_number,
                            uint64_t file_size, const Slice &k, void *arg,
                            void (*handle_result)(void *, const Slice &, const Slice &),
                            FileMetaData *meta, uint64_t lower, uint64_t upper, bool learned, Version *version) {

    adgMod::Stats* instance = adgMod::Stats::GetInstance();




//    static std::map<uint64_t, RandomAccessFile*> files_cache;
//    auto iter = files_cache.find(file_number);
//    if (iter == files_cache.end()) {
//        std::string filename = TableFileName(dbname_, file_number);
//        env_->NewRandomAccessFileLearned(filename, &file);
//        files_cache.emplace(file_number, file);
//    } else {
//        file = files_cache[file_number];
//    }

    // Get the file
#ifdef INTERNAL_TIMER
    instance->StartTimer(1);
#endif

    FilterAndFile* filter_and_file = nullptr;

    //instance->StartTimer(13);

    char buf[sizeof(file_number)];
    EncodeFixed64(buf, file_number);
    Slice cache_key(buf, sizeof(buf));
    Cache::Handle* cache_handle = file_cache->Lookup(cache_key);

    //instance->PauseTimer(13);

    if (cache_handle == nullptr) {
        RandomAccessFile* file = nullptr;
        FilterBlockReader* filter = nullptr;

        //instance->StartTimer(14);

        // Create new file
        std::string filename = TableFileName(dbname_, file_number);
        env_->NewRandomAccessFileLearned(filename, &file);

        //instance->PauseTimer(14);
        if (adgMod::use_filter) {

            //instance->StartTimer(15);

            // Load footer
            char footer_scratch[Footer::kEncodedLength];
            Slice footer_slice;
            Status s = file->Read(file_size - Footer::kEncodedLength, Footer::kEncodedLength, &footer_slice, footer_scratch);
            assert(s.ok());
            Footer footer;
            s = footer.DecodeFrom(&footer_slice);
            assert(s.ok());

            //instance->PauseTimer(15);

            if (options_.filter_policy != nullptr) {
                // Load meta index block

                //instance->StartTimer(16);

                BlockContents meta_contents;
                s = ReadBlock(file, options, footer.metaindex_handle(), &meta_contents);
                assert(s.ok());
                Block* meta_block = new Block(meta_contents);
                Iterator* meta_iter = meta_block->NewIterator(BytewiseComparator());
                string filter_name = "filter." + (string) options_.filter_policy->Name();
                meta_iter->Seek(filter_name);
                assert(meta_iter->Valid() && meta_iter->key() == filter_name);

                //instance->PauseTimer(16);
                //instance->StartTimer(17);

                // Load filter meta block
                Slice filter_handle_slice = meta_iter->value();
                BlockHandle filter_handle;
                s = filter_handle.DecodeFrom(&filter_handle_slice);
                assert(s.ok());
                BlockContents filter_contents;
                s = ReadBlock(file, options, filter_handle, &filter_contents);
                filter = new FilterBlockReader(options_.filter_policy, filter_contents.data);

                //instance->PauseTimer(17);
            }
        }

        //instance->StartTimer(18);

        // Insert Cache
        filter_and_file = new FilterAndFile();
        filter_and_file->file = file;
        filter_and_file->filter = filter;
        cache_handle = file_cache->Insert(cache_key, filter_and_file, 1, DeleteFilterAndFile);

        //instance->PauseTimer(18);
    } else {
        filter_and_file = reinterpret_cast<FilterAndFile*>(file_cache->Value(cache_handle));
    }

    RandomAccessFile* file = filter_and_file->file;
    FilterBlockReader* filter = filter_and_file->filter;

#ifdef INTERNAL_TIMER
    instance->PauseTimer(1);
#endif

    // Get the position we want to read
    size_t index_lower = lower / adgMod::block_num_entries;
    size_t index_upper = upper / adgMod::block_num_entries;
    for (uint64_t i = index_lower; i <= index_upper; ++i) {

#ifdef INTERNAL_TIMER
        instance->StartTimer(7);
#endif

        // Check Filter Block
        uint64_t block_offset = i * adgMod::block_size;
        if (filter != nullptr && !filter->KeyMayMatch(block_offset, k)) {
#ifdef INTERNAL_TIMER
            instance->PauseTimer(7);
#endif
            instance->IncrementCounter(5);
            continue;
        }

#ifdef INTERNAL_TIMER
        instance->PauseTimer(7);
        instance->StartTimer(5);
#endif

        size_t pos_block_lower = i == index_lower ? lower % adgMod::block_num_entries : 0;
        size_t pos_block_upper = i == index_upper ? upper % adgMod::block_num_entries : adgMod::block_num_entries - 1;

        // Read corresponding entries
        size_t read_size = (pos_block_upper - pos_block_lower + 1) * adgMod::entry_size;
        char scratch[read_size];
        Slice entries;
        file->Read(i * adgMod::block_size + pos_block_lower * adgMod::entry_size, read_size, &entries, scratch);

#ifdef INTERNAL_TIMER
        instance->PauseTimer(5);
        instance->StartTimer(3);
#endif

        // Binary Search
        uint64_t left = pos_block_lower, right = pos_block_upper;
        while (left < right) {
            uint32_t mid = (left + right) / 2;
            uint32_t shared, non_shared, value_length;
            const char* key_ptr = DecodeEntry(entries.data() + (mid - pos_block_lower) * adgMod::entry_size,
                    entries.data() + read_size, &shared, &non_shared, &value_length);
            if (key_ptr == nullptr || (shared != 0)) {
                assert(false && "Entry Corruption");
            }
            Slice mid_key(key_ptr, non_shared);
            int comp = adgMod::db->options_.comparator->Compare(mid_key, k);
            if (comp < 0) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }

        uint32_t shared, non_shared, value_length;
        const char* key_ptr = DecodeEntry(entries.data() + (left - pos_block_lower) * adgMod::entry_size,
                entries.data() + read_size, &shared, &non_shared, &value_length);
        if (key_ptr == nullptr || (shared != 0)) {
            assert(false && "Entry Corruption");
        }

#ifdef INTERNAL_TIMER
        instance->PauseTimer(3);
#endif

        Slice key(key_ptr, non_shared), value(key_ptr + non_shared, value_length);
        handle_result(arg, key, value);

        if (VersionSet::IsFound(arg)) break;
    }
    //delete file;
    file_cache->Release(cache_handle);
}



}  // namespace leveldb
