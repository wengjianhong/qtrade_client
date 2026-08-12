/// @file      grpc_config_bridge.hpp
/// @brief     经 gRPC 启动时拉取一次配置；拆成 EngineConfig / 策略列表 / 适配器参数
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_BRIDGE_GRPC_CONFIG_BRIDGE_HPP_
#define QTRADE_BRIDGE_GRPC_CONFIG_BRIDGE_HPP_

#include "qtrade/client/config_client/config_client.hpp"
#include "qtrade/common/config/service_config.hpp"

#include <qtrade/engine/engine.hpp>
#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/strategy/strategy.hpp>
#include <qtrade/structs/result.hpp>

#include <mutex>
#include <string>
#include <vector>

namespace qtrade::bridge {

/// @brief client 侧适配器选型（不注入引擎；由 SetQuoteApi/SetTraderApi 之前使用）
struct AdapterLaunchParams {
  /// 交易适配器类型：mock 或 emt
  std::string execution_adapter;
  /// 主行情源连接串
  std::string quote_connection_string;
};

/// @brief 启动时 GetEngineConfig 一次并缓存；策略与适配器参数与 EngineConfig 分离
class GrpcConfigBridge final {
 public:
  GrpcConfigBridge(qtrade::common::config::ServiceConfig service_config, std::string engine_id);
  ~GrpcConfigBridge();

  GrpcConfigBridge(const GrpcConfigBridge&) = delete;
  GrpcConfigBridge& operator=(const GrpcConfigBridge&) = delete;

  /// @brief 建连并 GetEngineConfig 一次缓存
  ErrorCode Init();

  /// @brief 关闭 client；可重复调用
  void Shutdown();

  /// @brief 读取已缓存的引擎运行配置（供 IEngine::Init）
  Result<qtrade::engine::EngineConfig> GetEngineConfig() const;

  /// @brief 读取已缓存的策略业务配置列表（供 AddStrategy）
  [[nodiscard]] std::vector<qtrade::strategy::StrategyConfig> GetStrategies() const;

  /// @brief 读取适配器选型参数（供 client 创建并 Connect 后 Set*Api）
  [[nodiscard]] AdapterLaunchParams GetAdapterLaunchParams() const;

 private:
  qtrade::common::config::ServiceConfig service_config_;
  std::string engine_id_;
  qtrade::client::ConfigClient client_;
  mutable std::mutex mutex_;
  bool has_config_ = false;
  qtrade::engine::EngineConfig cache_;
  std::vector<qtrade::strategy::StrategyConfig> strategies_;
  AdapterLaunchParams adapter_params_;
};

}  // namespace qtrade::bridge

#endif  // QTRADE_BRIDGE_GRPC_CONFIG_BRIDGE_HPP_
