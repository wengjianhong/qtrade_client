/// @file      bridge_convert.hpp
/// @brief     proto ↔ bridge 域模型转换
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_BRIDGE_BRIDGE_CONVERT_HPP_
#define QTRADE_BRIDGE_BRIDGE_CONVERT_HPP_

#include <qtrade/bridge/account_bridge.hpp>
#include <qtrade/bridge/account_risk_bridge.hpp>
#include <qtrade/common/proto/strategy_config_utils.hpp>
#include <qtrade/engine/engine.hpp>
#include <qtrade/proto/account/v1/account.pb.h>
#include <qtrade/proto/account_risk/v1/account_risk.pb.h>
#include <qtrade/proto/config/v1/config.pb.h>

#include <vector>

namespace qtrade::bridge {

[[nodiscard]] inline qtrade::engine::EngineConfig ToEngineConfig(const qtrade::config::v1::EngineConfig& proto) {
  qtrade::engine::EngineConfig out;
  out.engine_id = proto.engine_id();
  out.account_id = proto.account_id();
  out.quote_source = proto.quote_source();
  out.quote_failover = proto.quote_failover();
  if (proto.quote_max_stale_ms() > 0) {
    out.quote_max_stale_ms = proto.quote_max_stale_ms();
  }
  return out;
}

[[nodiscard]] inline std::vector<qtrade::strategy::StrategyConfig> ToStrategyConfigs(
  const qtrade::config::v1::EngineConfig& proto) {
  std::vector<qtrade::strategy::StrategyConfig> out;
  out.reserve(static_cast<std::size_t>(proto.strategies_size()));
  for (const auto& s : proto.strategies()) {
    out.push_back(qtrade::common::proto::ParseStrategyConfigProto(s));
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

[[nodiscard]] inline qtrade::account_risk::ReservationState ToReservationState(
  qtrade::account_risk::v1::ReserveOrderResponse::Decision d) {
  using Proto = qtrade::account_risk::v1::ReserveOrderResponse;
  using Dom = qtrade::account_risk::ReservationState;
  switch (d) {
    case Proto::APPROVED:
      return Dom::kReserved;
    case Proto::REJECTED:
      return Dom::kRejected;
    default:
      return Dom::kUnspecified;
  }
}

[[nodiscard]] inline qtrade::account_risk::ReservationState ToReservationState(const std::string& status) {
  using State = qtrade::account_risk::ReservationState;
  if (status == "reserved") {
    return State::kReserved;
  }
  if (status == "rejected") {
    return State::kRejected;
  }
  if (status == "released") {
    return State::kReleased;
  }
  if (status == "settled") {
    return State::kSettled;
  }
  if (status == "expired") {
    return State::kExpired;
  }
  return State::kUnspecified;
}

[[nodiscard]] inline qtrade::account_risk::v1::ReleaseOrderRequest::Reason ToProtoReleaseReason(
  qtrade::account_risk::ReleaseReason reason) {
  using Proto = qtrade::account_risk::v1::ReleaseOrderRequest;
  using Dom = qtrade::account_risk::ReleaseReason;
  switch (reason) {
    case Dom::kSendFailed:
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
  out.state = ToReservationState(proto.status());
  out.expires_at_unix_ms = proto.expires_at_unix_ms();
  return out;
}

}  // namespace qtrade::bridge

#endif  // QTRADE_BRIDGE_BRIDGE_CONVERT_HPP_
