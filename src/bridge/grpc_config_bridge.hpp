/// @file      grpc_config_bridge.hpp
/// @brief     IConfigBridge 的 gRPC 实现
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_BRIDGE_GRPC_CONFIG_BRIDGE_HPP_
#define QTRADE_BRIDGE_GRPC_CONFIG_BRIDGE_HPP_

#include "qtrade/client/config_client/config_client.hpp"
#include "qtrade/common/config/service_config.hpp"

#include <qtrade/bridge/config_bridge.hpp>

#include <mutex>
#include <string>

namespace qtrade::bridge {

/// @brief 经 ConfigClient 拉取/订阅；以 IConfigBridge 注入引擎前须先 Init()
class GrpcConfigBridge final : public qtrade::config::IConfigBridge {
 public:
  GrpcConfigBridge(qtrade::common::config::ServiceConfig service_config, std::string engine_id);
  ~GrpcConfigBridge() override;

  GrpcConfigBridge(const GrpcConfigBridge&) = delete;
  GrpcConfigBridge& operator=(const GrpcConfigBridge&) = delete;

  /// @brief 建连、首次 Get、启动 Subscribe（回调内 Apply）；注入引擎前调用
  ErrorCode Init();

  /// @brief 停止订阅并关闭 client；可重复调用
  void Shutdown();

  Result<qtrade::config::EngineConfig> GetEngineConfig() const override;
  ErrorCode ApplyEngineConfig(const qtrade::config::EngineConfig& config) override;

 private:
  qtrade::common::config::ServiceConfig service_config_;
  std::string engine_id_;
  qtrade::client::ConfigClient client_;
  mutable std::mutex mutex_;
  bool has_config_ = false;
  qtrade::config::EngineConfig cache_;
};

}  // namespace qtrade::bridge

#endif  // QTRADE_BRIDGE_GRPC_CONFIG_BRIDGE_HPP_
