// Copyright 2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#pragma once

#include <memory>
#include <queue>
#include <random>

#include "load_worker.h"

namespace triton { namespace perfanalyzer {

// TODO REFACTOR combine IConcurrencyWorker and IRequestRateWorker interfaces
// (must first combine threadconfigs)

/// Interface for ConcurrencyWorker
///
class IConcurrencyWorker {
 public:
  struct ThreadConfig {
    ThreadConfig(size_t thread_id)
        : thread_id_(thread_id), concurrency_(0),
          non_sequence_data_step_id_(thread_id), is_paused_(false)
    {
    }

    // ID of corresponding worker thread
    size_t thread_id_;
    // The concurrency level that the worker should produce
    size_t concurrency_;
    // The current data step id in case of non-sequence model
    size_t non_sequence_data_step_id_;
    // Whether or not the thread is issuing new inference requests
    bool is_paused_;
  };

  virtual void Infer(
      std::shared_ptr<ThreadStat> thread_stat,
      std::shared_ptr<ThreadConfig> thread_config) = 0;
};


/// Worker thread for the ConcurrencyManager
///
/// The worker maintains concurrency in different ways:
///   For sequence models, multiple contexts must be created for multiple
///   concurrent sequences.
///
///   For non-sequence models, one context can send out multiple requests
///   at the same time. Thus it uses one single context as every infer context
///   creates a worker thread implicitly.
///
class ConcurrencyWorker : public LoadWorker, public IConcurrencyWorker {
 public:
  virtual ~ConcurrencyWorker() = default;

  ConcurrencyWorker(
      const std::shared_ptr<ModelParser> parser,
      std::shared_ptr<DataLoader> data_loader, cb::BackendKind backend_kind,
      const std::shared_ptr<cb::ClientBackendFactory> factory,
      const size_t sequence_length, const uint64_t start_sequence_id,
      const uint64_t sequence_id_range, const bool on_sequence_model,
      const bool async, const size_t max_concurrency,
      const bool using_json_data, const bool streaming,
      const SharedMemoryType shared_memory_type, const int32_t batch_size,
      std::vector<std::shared_ptr<ThreadConfig>>& threads_config,
      std::vector<std::shared_ptr<SequenceStat>>& sequence_stat,
      std::unordered_map<std::string, SharedMemoryData>& shared_memory_regions,
      std::condition_variable& wake_signal, std::mutex& wake_mutex,
      size_t& active_threads, bool& execute, std::atomic<uint64_t>& curr_seq_id,
      std::uniform_int_distribution<uint64_t>& distribution)
      : LoadWorker(
            parser, data_loader, factory, sequence_stat, shared_memory_regions,
            backend_kind, shared_memory_type, on_sequence_model, async,
            streaming, batch_size, using_json_data, sequence_length,
            start_sequence_id, sequence_id_range, curr_seq_id, distribution,
            wake_signal, wake_mutex, execute),
        max_concurrency_(max_concurrency), threads_config_(threads_config),
        active_threads_(active_threads)
  {
  }

  void Infer(
      std::shared_ptr<ThreadStat> thread_stat,
      std::shared_ptr<ThreadConfig> thread_config);

 private:
  const size_t max_concurrency_;
  // TODO REFACTOR can we decouple this thread from the total count of threads?
  size_t& active_threads_;
  // TODO REFACTOR can we decouple this thread from every other thread?
  std::vector<std::shared_ptr<ThreadConfig>>& threads_config_;
};

}}  // namespace triton::perfanalyzer
