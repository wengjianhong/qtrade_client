/// @file      bridge_convert.hpp
/// @brief     proto ↔ bridge 域模型转换
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_BRIDGE_BRIDGE_CONVERT_HPP_
#define QTRADE_BRIDGE_BRIDGE_CONVERT_HPP_

#include <qtrade/bridge/account_bridge.hpp>
#include <qtrade/bridge/account_risk_bridge.hpp>
#include <qtrade/bridge/config_bridge.hpp>
#include <qtrade/common/proto/strategy_config_utils.hpp>
#include <qtrade/proto/account/v1/account.pb.h>
#include <qtrade/proto/account_risk/v1/account_risk.pb.h>
#include <qtrade/proto/config/v1/config.pb.h>

namespace qtrade::bridge {

[[nodiscard]] inline qtrade::config::EngineConfig ToEngineConfig(const qtrade::config::v1::EngineConfig& proto) {
  qtrade::config::EngineConfig out;
  out.engine_id = proto.engine_id();
  out.tenant_id = proto.tenant_id();
  out.account_id = proto.account_id();
  out.quote_source = proto.quote_source();
  out.quote_failover = proto.quote_failover();
  out.valid_until_unix_ms = proto.valid_until_unix_ms();
  out.execution_adapter = proto.execution_adapter();
  out.quote_connection_string = proto.quote_connection_string();
  out.version = proto.version();
  out.risk_budget.version = proto.risk_budget().version();
  out.risk_budget.max_notional = proto.risk_budget().max_notional();
  out.risk_budget.max_margin = proto.risk_budget().max_margin();
  out.risk_budget.max_open_orders = proto.risk_budget().max_open_orders();
  out.risk_budget.safety_buffer = proto.risk_budget().safety_buffer();
  out.strategies.reserve(static_cast<std::size_t>(proto.strategies_size()));
  for (const auto& s : proto.strategies()) {
    out.strategies.push_back(qtrade::common::proto::ParseStrategyConfigProto(s));
  }
  return out;
}

[[nodiscard]] inline qtrade::account::CredentialMaterial ToCredentialMaterial(
  const qtrade::account::v1::CredentialMaterial& proto) {
  qtrade::account::CredentialMaterial out;
  out.account_id = proto.account_id();
  out.broker_id = proto.broker_id();
  out.connection_string = proto.connection_string();
  out.password = proto.password();
  return out;
}

[[nodiscard]] inline qtrade::account_risk::AccountRiskPolicy ToAccountRiskPolicy(
  const qtrade::account_risk::v1::AccountRiskPolicy& proto) {
  qtrade::account_risk::AccountRiskPolicy out;
  out.account_id = proto.account_id();
  out.version = proto.version();
  out.valid_until_unix_ms = proto.valid_until_unix_ms();
  out.max_notional = proto.max_notional();
  out.max_margin = proto.max_margin();
  out.max_gross_exposure = proto.max_gross_exposure();
  out.max_open_orders = proto.max_open_orders();
  out.safety_buffer = proto.safety_buffer();
  out.enabled = proto.enabled();
  return out;
}

[[nodiscard]] inline qtrade::account_risk::ReserveDecision ToReserveDecision(
  qtrade::account_risk::v1::ReserveOrderResponse::Decision d) {
  using Proto = qtrade::account_risk::v1::ReserveOrderResponse;
  using Dom = qtrade::account_risk::ReserveDecision;
  switch (d) {
    case Proto::APPROVED:
      return Dom::kApproved;
    case Proto::REJECTED:
      return Dom::kRejected;
    case Proto::UNKNOWN:
      return Dom::kUnknown;
    default:
      return Dom::kUnspecified;
  }
}

[[nodiscard]] inline qtrade::account_risk::v1::ReleaseOrderRequest::Reason ToProtoReleaseReason(
  qtrade::account_risk::ReleaseReason reason) {
  using Proto = qtrade::account_risk::v1::ReleaseOrderRequest;
  using Dom = qtrade::account_risk::ReleaseReason;
  switch (reason) {
    case Dom::kEmsEnqueueFailed:
      return Proto::EMS_ENQUEUE_FAILED;
    case Dom::kRejectedByVenue:
      return Proto::REJECTED_BY_VENUE;
    case Dom::kCanceled:
      return Proto::CANCELED;
    case Dom::kSettled:
      return Proto::SETTLED;
    case Dom::kExpired:
      return Proto::EXPIRED;
    default:
      return Proto::REASON_UNSPECIFIED;
  }
}

[[nodiscard]] inline qtrade::account_risk::Reservation ToReservation(
  const qtrade::account_risk::v1::Reservation& proto) {
  qtrade::account_risk::Reservation out;
  out.order_id = proto.order_id();
  out.reservation_id = proto.reservation_id();
  out.status = proto.status();
  out.expires_at_unix_ms = proto.expires_at_unix_ms();
  return out;
}

}  // namespace qtrade::bridge

#endif  // QTRADE_BRIDGE_BRIDGE_CONVERT_HPP_
