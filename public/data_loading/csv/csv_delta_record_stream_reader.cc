/*
 * Copyright 2022 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "public/data_loading/csv/csv_delta_record_stream_reader.h"

#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "glog/logging.h"
#include "public/data_loading/records_utils.h"

namespace kv_server {
namespace {
absl::StatusOr<int64_t> GetLogicalCommitTime(
    absl::string_view logical_commit_time) {
  if (int64_t result; absl::SimpleAtoi(logical_commit_time, &result)) {
    return result;
  }
  return absl::InvalidArgumentError(absl::StrCat(
      "Cannot convert timestamp:", logical_commit_time, " to a number."));
}

absl::StatusOr<KeyValueMutationType> GetDeltaMutationType(
    absl::string_view mutation_type) {
  std::string mt_lower = absl::AsciiStrToLower(mutation_type);
  if (mt_lower == kUpdateMutationType) {
    return KeyValueMutationType::Update;
  }
  if (mt_lower == kDeleteMutationType) {
    return KeyValueMutationType::Delete;
  }
  return absl::InvalidArgumentError(
      absl::StrCat("Unknown mutation type:", mutation_type));
}

absl::StatusOr<KeyValueMutationRecordValueT> GetRecordValue(
    const riegeli::CsvRecord& csv_record, char value_separator) {
  auto type = absl::AsciiStrToLower(csv_record[kValueTypeColumn]);
  if (kValueTypeString == type) {
    return csv_record[kValueColumn];
  }
  if (kValueTypeStringSet == type) {
    return absl::StrSplit(csv_record[kValueColumn], value_separator);
  }
  return absl::InvalidArgumentError(
      absl::StrCat("Value type: ", type, " is not supported"));
}

absl::StatusOr<DataRecordStruct> MakeDeltaFileRecordStructWithKVMutation(
    const riegeli::CsvRecord& csv_record, char value_separator) {
  KeyValueMutationRecordStruct record;
  record.key = csv_record[kKeyColumn];
  auto value = GetRecordValue(csv_record, value_separator);
  if (!value.ok()) {
    return value.status();
  }
  record.value = *value;
  absl::StatusOr<int64_t> commit_time =
      GetLogicalCommitTime(csv_record[kLogicalCommitTimeColumn]);
  if (!commit_time.ok()) {
    return commit_time.status();
  }
  record.logical_commit_time = *commit_time;
  absl::StatusOr<KeyValueMutationType> mutation_type =
      GetDeltaMutationType(csv_record[kMutationTypeColumn]);
  if (!mutation_type.ok()) {
    return mutation_type.status();
  }
  record.mutation_type = *mutation_type;

  DataRecordStruct data_record;
  data_record.record = record;
  return data_record;
}

absl::StatusOr<UserDefinedFunctionsLanguage> GetUdfLanguage(
    const riegeli::CsvRecord& csv_record) {
  auto language = absl::AsciiStrToLower(csv_record[kLanguageColumn]);
  if (kLanguageJavascript == language) {
    return UserDefinedFunctionsLanguage::Javascript;
  }
  return absl::InvalidArgumentError(
      absl::StrCat("Language: ", language, " is not supported."));
}

absl::StatusOr<DataRecordStruct> MakeDeltaFileRecordStructWithUdfConfig(
    const riegeli::CsvRecord& csv_record) {
  UserDefinedFunctionsConfigStruct udf_config;
  udf_config.code_snippet = csv_record[kCodeSnippetColumn];
  udf_config.handler_name = csv_record[kHandlerNameColumn];

  absl::StatusOr<int64_t> commit_time =
      GetLogicalCommitTime(csv_record[kLogicalCommitTimeColumn]);
  if (!commit_time.ok()) {
    return commit_time.status();
  }
  udf_config.logical_commit_time = *commit_time;

  auto language = GetUdfLanguage(csv_record);
  if (!language.ok()) {
    return language.status();
  }
  udf_config.language = *language;

  DataRecordStruct data_record;
  data_record.record = udf_config;
  return data_record;
}

}  // namespace

namespace internal {
absl::StatusOr<DataRecordStruct> MakeDeltaFileRecordStruct(
    const riegeli::CsvRecord& csv_record, const DataRecordType& record_type,
    char value_separator) {
  switch (record_type) {
    case DataRecordType::kKeyValueMutationRecord:
      return MakeDeltaFileRecordStructWithKVMutation(csv_record,
                                                     value_separator);
    case DataRecordType::kUserDefinedFunctionsConfig:
      return MakeDeltaFileRecordStructWithUdfConfig(csv_record);
    default:
      return absl::InvalidArgumentError("Invalid record type.");
  }
}
}  // namespace internal

}  // namespace kv_server
