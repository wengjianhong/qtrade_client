/// @file      grpc_config_bridge.cpp
/// @brief     IConfigBridge 的 gRPC 实现
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/bridge/grpc_config_bridge.hpp"

#include "qtrade/bridge/bridge_convert.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::bridge {

GrpcConfigBridge::GrpcConfigBridge(qtrade::common::config::ServiceConfig service_config, std::string engine_id)
  : service_config_(std::move(service_config)), engine_id_(std::move(engine_id)) {}

GrpcConfigBridge::~GrpcConfigBridge() {
  Shutdown();
}

ErrorCode GrpcConfigBridge::Init() {
  qtrade::client::ConfigClientOptions options;
  options.service_config = service_config_;
  if (const auto rc = client_.Init(options); rc != ErrorCode::kSuccess) {
    return rc;
  }

  qtrade::config::v1::GetEngineConfigRequest get_request;
  get_request.set_engine_id(engine_id_);
  qtrade::config::v1::GetEngineConfigResponse get_response;
  if (const auto rc = client_.GetEngineConfig(get_request, get_response); rc != ErrorCode::kSuccess) {
    spdlog::error("[GrpcConfigBridge] GetEngineConfig failed");
    return rc;
  }

  const auto config = ToEngineConfig(get_response.engine());
  if (config.version == 0) {
    spdlog::error("[GrpcConfigBridge] invalid engine config version=0");
    return ErrorCode::kInvalidArgument;
  }

  std::lock_guard lock(mutex_);
  cache_ = config;
  has_config_ = true;
  return ErrorCode::kSuccess;
}

void GrpcConfigBridge::Shutdown() {
  client_.Shutdown();
}

Result<qtrade::config::EngineConfig> GrpcConfigBridge::GetEngineConfig() const {
  Result<qtrade::config::EngineConfig> result;
  std::lock_guard lock(mutex_);
  if (!has_config_) {
    result.error_code = ErrorCode::kNotInitialized;
    result.error_message = "engine config not available";
    return result;
  }
  result.data = cache_;
  return result;
}

}  // namespace qtrade::bridge
