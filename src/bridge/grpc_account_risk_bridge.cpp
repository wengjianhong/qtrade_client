/// @file      grpc_account_risk_bridge.cpp
/// @brief     IAccountRiskBridge 的 gRPC 实现
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/bridge/grpc_account_risk_bridge.hpp"

#include "qtrade/bridge/bridge_convert.hpp"

namespace qtrade::bridge {

GrpcAccountRiskBridge::GrpcAccountRiskBridge(qtrade::common::config::ServiceConfig service_config)
  : service_config_(std::move(service_config)) {}

GrpcAccountRiskBridge::~GrpcAccountRiskBridge() {
  Shutdown();
}

ErrorCode GrpcAccountRiskBridge::Init() {
  qtrade::client::AccountRiskClientOptions options;
  options.service_config = service_config_;
  return client_.Init(options);
}

void GrpcAccountRiskBridge::Shutdown() {
  client_.Shutdown();
}

std::string GrpcAccountRiskBridge::CacheKey(const std::string& account_id) {
  return account_id;
}

Result<qtrade::account_risk::AccountRiskPolicy> GrpcAccountRiskBridge::GetAccountRiskPolicy(
  const std::string& account_id) const {
  Result<qtrade::account_risk::AccountRiskPolicy> result;

  if (client_.IsInitialized()) {
    qtrade::account_risk::v1::GetAccountRiskPolicyRequest request;
    request.set_account_id(account_id);
    qtrade::account_risk::v1::GetAccountRiskPolicyResponse response;
    const auto rc = client_.GetAccountRiskPolicy(request, response);
    if (rc == ErrorCode::kSuccess) {
      auto policy = ToAccountRiskPolicy(response.policy());
      {
        std::lock_guard lock(mutex_);
        policy_cache_[CacheKey(account_id)] = policy;
      }
      result.data = std::move(policy);
      return result;
    }
    if (rc == ErrorCode::kNotFound) {
      result.error_code = ErrorCode::kNotFound;
      result.error_message = "account risk policy not found";
      return result;
    }
    result.error_code = ErrorCode::kTimeout;
    result.error_message = "GetAccountRiskPolicy RPC failed";
  }

  std::lock_guard lock(mutex_);
  const auto it = policy_cache_.find(CacheKey(account_id));
  if (it == policy_cache_.end()) {
    if (result.error_code == ErrorCode::kSuccess) {
      result.error_code = ErrorCode::kNotInitialized;
      result.error_message = "account risk policy not available";
    }
    return result;
  }
  result.error_code = ErrorCode::kSuccess;
  result.error_message.clear();
  result.data = it->second;
  return result;
}

Result<qtrade::account_risk::Reservation> GrpcAccountRiskBridge::Reserve(
  const qtrade::account_risk::ReserveRequest& reserve_request) {
  Result<qtrade::account_risk::Reservation> result;
  if (!client_.IsInitialized()) {
    result.error_code = ErrorCode::kNotInitialized;
    return result;
  }

  qtrade::account_risk::v1::ReserveOrderRequest request;
  request.set_account_id(reserve_request.account_id);
  request.set_risk_config_version(reserve_request.expected_policy_version);
  request.set_reservation_ttl_ms(reserve_request.ttl_ms);
  auto* proto_intent = request.mutable_intent();
  proto_intent->set_order_id(reserve_request.order_id);
  proto_intent->set_engine_id(reserve_request.exposure.engine_id);
  proto_intent->set_strategy_id(reserve_request.exposure.strategy_id);
  proto_intent->set_instrument_id(reserve_request.exposure.instrument_id);
  proto_intent->set_side(std::to_string(static_cast<int>(reserve_request.exposure.side)));
  proto_intent->set_price(reserve_request.exposure.price);
  proto_intent->set_quantity(reserve_request.exposure.quantity);
  proto_intent->set_estimated_notional(reserve_request.exposure.notional);
  proto_intent->set_estimated_margin(reserve_request.exposure.margin);

  qtrade::account_risk::v1::ReserveOrderResponse response;
  if (const auto rc = client_.ReserveOrder(request, response); rc != ErrorCode::kSuccess) {
    result.error_code = rc;
    result.error_message = "ReserveOrder RPC failed";
    return result;
  }

  qtrade::account_risk::Reservation out;
  out.account_id = reserve_request.account_id;
  out.order_id = reserve_request.order_id;
  out.state = ToReservationState(response.decision());
  out.reason = response.reject_reason();
  out.policy_version = response.policy_version();
  out.reservation_id = response.reservation_id();
  out.expires_at_unix_ms = response.expires_at_unix_ms();
  result.data = std::move(out);
  return result;
}

Result<qtrade::account_risk::Reservation> GrpcAccountRiskBridge::Release(
  const qtrade::account_risk::ReleaseRequest& release_request) {
  Result<qtrade::account_risk::Reservation> result;
  if (!client_.IsInitialized()) {
    result.error_code = ErrorCode::kNotInitialized;
    return result;
  }

  qtrade::account_risk::v1::ReleaseOrderRequest request;
  request.set_account_id(release_request.account_id);
  request.set_order_id(release_request.order_id);
  request.set_reason(ToProtoReleaseReason(release_request.reason));
  request.set_settled_notional(release_request.notional);
  request.set_settled_margin(release_request.margin);

  qtrade::account_risk::v1::ReleaseOrderResponse response;
  if (const auto rc = client_.ReleaseOrder(request, response); rc != ErrorCode::kSuccess) {
    result.error_code = rc;
    result.error_message = "ReleaseOrder RPC failed";
    return result;
  }

  qtrade::account_risk::Reservation out;
  out.account_id = release_request.account_id;
  out.order_id = release_request.order_id;
  if (response.released()) {
    out.state = release_request.reason == qtrade::account_risk::ReleaseReason::kSettled
                  ? qtrade::account_risk::ReservationState::kSettled
                  : qtrade::account_risk::ReservationState::kReleased;
  }
  out.reason = response.reject_reason();
  result.data = std::move(out);
  return result;
}

Result<qtrade::account_risk::Reservation> GrpcAccountRiskBridge::QueryReservation(const std::string& account_id,
                                                                                  const std::string& order_id) const {
  Result<qtrade::account_risk::Reservation> result;
  if (!client_.IsInitialized()) {
    result.error_code = ErrorCode::kNotInitialized;
    return result;
  }

  qtrade::account_risk::v1::GetReservationRequest request;
  request.set_account_id(account_id);
  request.set_order_id(order_id);
  qtrade::account_risk::v1::GetReservationResponse response;
  if (const auto rc = client_.QueryReservation(request, response); rc != ErrorCode::kSuccess) {
    result.error_code = rc;
    result.error_message = "QueryReservation RPC failed";
    return result;
  }
  auto reservation = ToReservation(response.reservation());
  reservation.account_id = account_id;
  result.data = std::move(reservation);
  return result;
}

}  // namespace qtrade::bridge
