/// @file      config_client.cpp
/// @brief     ConfigService gRPC 客户端实现
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/client/config_client/config_client.hpp"

#include <qtrade/proto/config/v1/config.grpc.pb.h>

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

#include <chrono>

namespace qtrade::client {
namespace {

[[nodiscard]] std::chrono::system_clock::time_point DeadlineFrom(const qtrade::common::config::ServiceConfig& cfg) {
  const int timeout_ms = cfg.timeout_ms > 0 ? cfg.timeout_ms : 5000;
  return std::chrono::system_clock::now() + std::chrono::milliseconds(timeout_ms);
}

}  // namespace

struct ConfigClient::Impl {
  ConfigClientOptions options;
  std::shared_ptr<grpc::Channel> channel;
  std::unique_ptr<qtrade::config::v1::ConfigService::Stub> stub;
  bool initialized = false;
};

ConfigClient::ConfigClient() : impl_(std::make_unique<Impl>()) {}

ConfigClient::~ConfigClient() {
  Shutdown();
}

ErrorCode ConfigClient::Init(const ConfigClientOptions& options) {
  if (impl_->initialized) {
    return ErrorCode::kSystemError;
  }
  if (options.service_config.host.empty() || options.service_config.port <= 0) {
    return ErrorCode::kInternalError;
  }
  impl_->options = options;
  impl_->channel = grpc::CreateChannel(options.service_config.Address(), grpc::InsecureChannelCredentials());
  impl_->stub = qtrade::config::v1::ConfigService::NewStub(impl_->channel);
  impl_->initialized = true;
  return ErrorCode::kSuccess;
}

bool ConfigClient::IsInitialized() const {
  return impl_->initialized;
}

void ConfigClient::Shutdown() {
  impl_->stub.reset();
  impl_->channel.reset();
  impl_->initialized = false;
}

ErrorCode ConfigClient::GetEngineConfig(const qtrade::config::v1::GetEngineConfigRequest& request,
                                        qtrade::config::v1::GetEngineConfigResponse& response) {
  if (!impl_->initialized || !impl_->stub) {
    return ErrorCode::kNotInitialized;
  }
  grpc::ClientContext context;
  context.set_deadline(DeadlineFrom(impl_->options.service_config));
  const grpc::Status status = impl_->stub->GetEngineConfig(&context, request, &response);
  if (!status.ok()) {
    spdlog::warn("[ConfigClient] GetEngineConfig failed: {}", status.error_message());
    return ErrorCode::kTimeout;
  }
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::client
