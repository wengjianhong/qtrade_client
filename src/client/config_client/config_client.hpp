/// @file      config_client.hpp
/// @brief     config-service gRPC 客户端（薄封装）
/// @details   启动时 GetEngineConfig；不订阅运行时推送。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_CLIENT_CONFIG_CLIENT_HPP_
#define QTRADE_TRADING_CLIENT_CONFIG_CLIENT_HPP_

#include "qtrade/common/config/service_config.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/proto/config/v1/config.pb.h>

#include <memory>

namespace qtrade::client {

/// @brief ConfigClient 初始化选项
struct ConfigClientOptions {
  /// config-service 连出端点
  qtrade::common::config::ServiceConfig service_config;
};

/// @brief ConfigService gRPC 客户端
class ConfigClient {
 public:
  ConfigClient();
  ~ConfigClient();
  ConfigClient(const ConfigClient&) = delete;
  ConfigClient& operator=(const ConfigClient&) = delete;

  /// @brief 建立 gRPC 通道与 stub
  ErrorCode Init(const ConfigClientOptions& options);

  /// @brief 是否已 Init
  [[nodiscard]] bool IsInitialized() const;

  /// @brief 释放资源
  void Shutdown();

  /// @brief 获取引擎配置
  ErrorCode GetEngineConfig(const qtrade::config::v1::GetEngineConfigRequest& request,
                            qtrade::config::v1::GetEngineConfigResponse& response);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace qtrade::client

#endif  // QTRADE_TRADING_CLIENT_CONFIG_CLIENT_HPP_
