/// @file      grpc_account_bridge.hpp
/// @brief     IAccountBridge 的 gRPC 实现
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_BRIDGE_GRPC_ACCOUNT_BRIDGE_HPP_
#define QTRADE_BRIDGE_GRPC_ACCOUNT_BRIDGE_HPP_

#include "qtrade/client/account_client/account_client.hpp"
#include "qtrade/common/config/service_config.hpp"

#include <qtrade/bridge/account_bridge.hpp>

#include <mutex>
#include <string>
#include <unordered_map>

namespace qtrade::bridge {

/// @brief 经 AccountClient 取凭证；以 IAccountBridge 注入引擎前须先 Init()
class GrpcAccountBridge final : public qtrade::account::IAccountBridge {
 public:
  explicit GrpcAccountBridge(qtrade::common::config::ServiceConfig service_config);
  ~GrpcAccountBridge() override;

  GrpcAccountBridge(const GrpcAccountBridge&) = delete;
  GrpcAccountBridge& operator=(const GrpcAccountBridge&) = delete;

  /// @brief 初始化 gRPC client；注入引擎前调用
  ErrorCode Init();

  /// @brief 关闭 client；可重复调用
  void Shutdown();

  Result<qtrade::account::CredentialMaterial> GetCredential(const std::string& tenant_id,
                                                            const std::string& account_id,
                                                            const std::string& engine_id) const override;
  ErrorCode ApplyCredential(const qtrade::account::CredentialMaterial& credential) override;

 private:
  static std::string CacheKey(const std::string& tenant_id, const std::string& account_id);

  qtrade::common::config::ServiceConfig service_config_;
  mutable qtrade::client::AccountClient client_;
  mutable std::mutex mutex_;
  mutable std::unordered_map<std::string, qtrade::account::CredentialMaterial> cache_;
};

}  // namespace qtrade::bridge

#endif  // QTRADE_BRIDGE_GRPC_ACCOUNT_BRIDGE_HPP_
