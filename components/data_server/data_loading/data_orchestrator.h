// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef COMPONENTS_DATA_SERVER_DATA_LOADING_DATA_ORCHESTRATOR_H_
#define COMPONENTS_DATA_SERVER_DATA_LOADING_DATA_ORCHESTRATOR_H_

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "absl/functional/any_invocable.h"
#include "absl/status/status.h"
#include "components/data/blob_storage/blob_storage_change_notifier.h"
#include "components/data/blob_storage/blob_storage_client.h"
#include "components/data/blob_storage/delta_file_notifier.h"
#include "components/data/realtime/delta_file_record_change_notifier.h"
#include "components/data/realtime/realtime_notifier.h"
#include "components/data_server/cache/cache.h"
#include "components/udf/udf_client.h"
#include "public/data_loading/readers/riegeli_stream_io.h"
#include "src/cpp/telemetry/metrics_recorder.h"

namespace kv_server {

// Coordinate data loading.
//
// This class is intended to be used in a single thread.
class DataOrchestrator {
 public:
  // Stops loading new data, if it is currently continuously loading new data.
  virtual ~DataOrchestrator() = default;

  struct RealtimeOptions {
    std::unique_ptr<DeltaFileRecordChangeNotifier>
        delta_file_record_change_notifier;
    std::unique_ptr<RealtimeNotifier> realtime_notifier;
  };
  struct Options {
    // bucket to keep loading data from.
    const std::string data_bucket;
    Cache& cache;
    BlobStorageClient& blob_client;
    DeltaFileNotifier& delta_notifier;
    BlobStorageChangeNotifier& change_notifier;
    UdfClient& udf_client;
    StreamRecordReaderFactory<std::string_view>& delta_stream_reader_factory;
    std::vector<RealtimeOptions>& realtime_options;
    const int32_t shard_num = 0;
    const int32_t num_shards = 1;
  };

  // Creates initial state. Scans the bucket and initializes the cache with data
  // read from the files in the bucket.
  static absl::StatusOr<std::unique_ptr<DataOrchestrator>> TryCreate(
      Options options,
      privacy_sandbox::server_common::MetricsRecorder& metrics_recorder);

  // Starts a separate thread to monitor and load new data until the returned
  // this object is destructed.
  // Returns immediately without blocking.
  virtual absl::Status Start() = 0;
};
}  // namespace kv_server

#endif  // COMPONENTS_DATA_SERVER_DATA_LOADING_DATA_ORCHESTRATOR_H_
