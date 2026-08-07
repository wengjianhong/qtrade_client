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
  if (const auto rc = ApplyEngineConfig(ToEngineConfig(get_response.engine())); rc != ErrorCode::kSuccess) {
    return rc;
  }

  qtrade::config::v1::SubscribeEngineConfigRequest subscribe_request;
  subscribe_request.set_engine_id(engine_id_);
  {
    std::lock_guard lock(mutex_);
    subscribe_request.set_since_version(cache_.version);
  }
  const auto on_subscribe = [this](const qtrade::config::v1::SubscribeEngineConfigResponse& response) {
    (void)ApplyEngineConfig(ToEngineConfig(response.engine()));
  };
  if (const auto rc = client_.SubscribeEngineConfig(subscribe_request, on_subscribe); rc != ErrorCode::kSuccess) {
    spdlog::warn("[GrpcConfigBridge] SubscribeEngineConfig failed: will rely on cached Get");
  }
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

ErrorCode GrpcConfigBridge::ApplyEngineConfig(const qtrade::config::EngineConfig& config) {
  std::lock_guard lock(mutex_);
  if (has_config_ && config.version != 0 && config.version < cache_.version) {
    return ErrorCode::kInvalidArgument;
  }
  cache_ = config;
  has_config_ = true;
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::bridge
