/// @file      grpc_config_bridge.cpp
/// @brief     经 gRPC 启动时拉取一次配置
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

  const auto& proto = get_response.engine();
  const auto config = ToEngineConfig(proto);
  if (config.engine_id.empty() || config.account_id.empty()) {
    spdlog::error("[GrpcConfigBridge] invalid engine config: empty engine_id/account_id");
    return ErrorCode::kInvalidArgument;
  }

  AdapterLaunchParams adapter_params;
  adapter_params.execution_adapter = proto.execution_adapter();
  adapter_params.quote_connection_string = proto.quote_connection_string();

  auto strategies = ToStrategyConfigs(proto);

  std::lock_guard lock(mutex_);
  cache_ = config;
  strategies_ = std::move(strategies);
  adapter_params_ = std::move(adapter_params);
  has_config_ = true;
  return ErrorCode::kSuccess;
}

void GrpcConfigBridge::Shutdown() {
  client_.Shutdown();
}

Result<qtrade::engine::EngineConfig> GrpcConfigBridge::GetEngineConfig() const {
  Result<qtrade::engine::EngineConfig> result;
  std::lock_guard lock(mutex_);
  if (!has_config_) {
    result.error_code = ErrorCode::kNotInitialized;
    result.error_message = "engine config not available";
    return result;
  }
  result.data = cache_;
  return result;
}

std::vector<qtrade::strategy::StrategyConfig> GrpcConfigBridge::GetStrategies() const {
  std::lock_guard lock(mutex_);
  return strategies_;
}

AdapterLaunchParams GrpcConfigBridge::GetAdapterLaunchParams() const {
  std::lock_guard lock(mutex_);
  return adapter_params_;
}

}  // namespace qtrade::bridge
