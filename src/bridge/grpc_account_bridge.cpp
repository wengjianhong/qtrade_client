/// @file      grpc_account_bridge.cpp
/// @brief     IAccountBridge 的 gRPC 实现
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/bridge/grpc_account_bridge.hpp"

#include "qtrade/bridge/bridge_convert.hpp"

namespace qtrade::bridge {

GrpcAccountBridge::GrpcAccountBridge(qtrade::common::config::ServiceConfig service_config)
  : service_config_(std::move(service_config)) {}

GrpcAccountBridge::~GrpcAccountBridge() {
  Shutdown();
}

ErrorCode GrpcAccountBridge::Init() {
  qtrade::client::AccountClientOptions options;
  options.service_config = service_config_;
  return client_.Init(options);
}

void GrpcAccountBridge::Shutdown() {
  client_.Shutdown();
}

std::string GrpcAccountBridge::CacheKey(const std::string& account_id) {
  return account_id;
}

Result<qtrade::account::CredentialMaterial> GrpcAccountBridge::GetCredential(const std::string& account_id,
                                                                             const std::string& engine_id) const {
  Result<qtrade::account::CredentialMaterial> result;

  if (client_.IsInitialized()) {
    qtrade::account::v1::GetCredentialRequest request;
    request.set_account_id(account_id);
    request.set_engine_id(engine_id);
    qtrade::account::v1::GetCredentialResponse response;
    if (const auto rc = client_.GetCredential(request, response); rc == ErrorCode::kSuccess) {
      auto material = ToCredentialMaterial(response.credential());
      {
        std::lock_guard lock(mutex_);
        cache_[CacheKey(account_id)] = material;
      }
      result.data = std::move(material);
      return result;
    }
    result.error_code = ErrorCode::kTimeout;
    result.error_message = "GetCredential RPC failed";
  }

  std::lock_guard lock(mutex_);
  const auto it = cache_.find(CacheKey(account_id));
  if (it == cache_.end()) {
    if (result.error_code == ErrorCode::kSuccess) {
      result.error_code = ErrorCode::kNotInitialized;
      result.error_message = "credential not available";
    }
    return result;
  }
  result.error_code = ErrorCode::kSuccess;
  result.error_message.clear();
  result.data = it->second;
  return result;
}

}  // namespace qtrade::bridge
