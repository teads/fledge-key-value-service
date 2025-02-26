/*
 * Copyright 2023 Google LLC
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

#ifndef COMPONENTS_UDF_LOGGING_HOOK_H_
#define COMPONENTS_UDF_LOGGING_HOOK_H_

#include <string>
#include <tuple>

#include "glog/logging.h"

namespace kv_server {

// UDF hook for logging a string.
// TODO(b/285331079): Disable for production builds.
inline std::string LogMessage(std::tuple<std::string>& input) {
  LOG(INFO) << std::get<0>(input);
  // void is not allowed as an output type, so return an empty string.
  return "";
}

}  // namespace kv_server

#endif  // COMPONENTS_UDF_LOGGING_HOOK_H_
