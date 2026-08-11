/// @file      grpc_account_risk_bridge.hpp
/// @brief     IAccountRiskBridge 的 gRPC 实现
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_BRIDGE_GRPC_ACCOUNT_RISK_BRIDGE_HPP_
#define QTRADE_BRIDGE_GRPC_ACCOUNT_RISK_BRIDGE_HPP_

#include "qtrade/client/account_risk_client/account_risk_client.hpp"
#include "qtrade/common/config/service_config.hpp"

#include <qtrade/bridge/account_risk_bridge.hpp>

#include <mutex>
#include <string>
#include <unordered_map>

namespace qtrade::bridge {

/// @brief 经 AccountRiskClient 做硬风控；以 IAccountRiskBridge 注入引擎前须先 Init()
class GrpcAccountRiskBridge final : public qtrade::account_risk::IAccountRiskBridge {
 public:
  explicit GrpcAccountRiskBridge(qtrade::common::config::ServiceConfig service_config);
  ~GrpcAccountRiskBridge() override;

  GrpcAccountRiskBridge(const GrpcAccountRiskBridge&) = delete;
  GrpcAccountRiskBridge& operator=(const GrpcAccountRiskBridge&) = delete;

  /// @brief 初始化 gRPC client；注入引擎前调用
  ErrorCode Init();

  /// @brief 关闭 client；可重复调用
  void Shutdown();

  Result<qtrade::account_risk::AccountRiskPolicy> GetAccountRiskPolicy(
    const std::string& account_id) const override;

  Result<qtrade::account_risk::ReserveOrderResult> ReserveOrder(const std::string& account_id,
                                                                const qtrade::account_risk::OrderIntent& intent,
                                                                std::uint64_t risk_config_version,
                                                                std::int64_t reservation_ttl_ms) override;

  Result<qtrade::account_risk::ReleaseOrderResult> ReleaseOrder(const std::string& account_id,
                                                                const std::string& order_id,
                                                                qtrade::account_risk::ReleaseReason reason,
                                                                double settled_notional,
                                                                double settled_margin) override;

  Result<qtrade::account_risk::Reservation> GetReservation(const std::string& account_id,
                                                           const std::string& order_id) const override;

 private:
  static std::string CacheKey(const std::string& account_id);

  qtrade::common::config::ServiceConfig service_config_;
  mutable qtrade::client::AccountRiskClient client_;
  mutable std::mutex mutex_;
  mutable std::unordered_map<std::string, qtrade::account_risk::AccountRiskPolicy> policy_cache_;
};

}  // namespace qtrade::bridge

#endif  // QTRADE_BRIDGE_GRPC_ACCOUNT_RISK_BRIDGE_HPP_
