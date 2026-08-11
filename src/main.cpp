/// @file      main.cpp
/// @brief     企业级交易引擎客户端入口（qtrade_client）
/// @details   依赖已安装的 qtrade_engine（IEngine）与 qtrade_service（proto）；
///            本仓持有 Grpc*Bridge / 适配器，就绪后注入引擎。
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
#include "adapters/emt/emt_adapter_factory.hpp"
#include "adapters/mock/mock_adapter_factory.hpp"
#include "qtrade/bridge/grpc_account_bridge.hpp"
#include "qtrade/bridge/grpc_account_risk_bridge.hpp"
#include "qtrade/bridge/grpc_config_bridge.hpp"
#include "qtrade/common/boot/process_boot.hpp"
#include "qtrade/common/system/signal.hpp"
#include "qtrade/common/system/systemd_notify.hpp"
#include "qtrade/engine/core/engine_boot.hpp"
#include "qtrade/engine/trading_engine_define.hpp"

#include <qtrade/engine/engine.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace {

struct AdapterBundle {
  std::unique_ptr<qtrade_sdk::quote::QuoteApi> quote_api;
  std::unique_ptr<qtrade_sdk::trader::TraderApi> trader_api;
};

[[nodiscard]] bool ConnectAndTake(AdapterBundle* out,
                                  std::unique_ptr<qtrade_sdk::quote::QuoteApi> quote_api,
                                  std::unique_ptr<qtrade_sdk::trader::TraderApi> trader_api,
                                  qtrade_sdk::quote::ConnectRequest quote_request,
                                  qtrade_sdk::trader::ConnectRequest trader_request) {
  if (quote_api->Connect(quote_request) != qtrade::ErrorCode::kSuccess) {
    return false;
  }
  if (trader_api->Connect(trader_request) != qtrade::ErrorCode::kSuccess) {
    quote_api->Disconnect();
    return false;
  }
  out->quote_api = std::move(quote_api);
  out->trader_api = std::move(trader_api);
  return true;
}

[[nodiscard]] bool BuildAdapters(const qtrade::common::config::QtradeEngineBootstrapConfig& bootstrap,
                                 qtrade::config::IConfigBridge* config_bridge,
                                 qtrade::account::IAccountBridge* account_bridge,
                                 AdapterBundle* out) {
  if (config_bridge == nullptr) {
    // 无远端配置时默认 mock（本地 / 开发）
    auto bundle = qtrade::adapter::mock::CreateMockAdapters("mock://local", bootstrap.config.identity.account_id);
    return ConnectAndTake(out,
                          std::move(bundle.quote_api),
                          std::move(bundle.trader_api),
                          std::move(bundle.quote_request),
                          std::move(bundle.trader_request));
  }

  const auto cfg_result = config_bridge->GetEngineConfig();
  if (cfg_result.error_code != qtrade::ErrorCode::kSuccess || !cfg_result.data.has_value()) {
    return false;
  }
  const auto& runtime = *cfg_result.data;
  if (runtime.execution_adapter.empty() || runtime.quote_connection_string.empty()) {
    return false;
  }

  if (runtime.execution_adapter == "mock") {
    auto bundle =
      qtrade::adapter::mock::CreateMockAdapters(runtime.quote_connection_string, bootstrap.config.identity.account_id);
    return ConnectAndTake(out,
                          std::move(bundle.quote_api),
                          std::move(bundle.trader_api),
                          std::move(bundle.quote_request),
                          std::move(bundle.trader_request));
  }
  if (runtime.execution_adapter == "emt") {
    if (account_bridge == nullptr) {
      return false;
    }
    qtrade::ErrorCode create_error = qtrade::ErrorCode::kInternalError;
    auto bundle = qtrade::adapter::emt::CreateEmtAdapters(*account_bridge,
                                                          bootstrap.config.identity.engine_id,
                                                          bootstrap.config.identity.account_id,
                                                          runtime.quote_connection_string,
                                                          &create_error);
    if (!bundle.has_value()) {
      return false;
    }
    return ConnectAndTake(out,
                          std::move(bundle->quote_api),
                          std::move(bundle->trader_api),
                          std::move(bundle->quote_request),
                          std::move(bundle->trader_request));
  }
  return false;
}

}  // namespace

int main(int argc, char** argv) {
  qtrade::common::system::BlockInterruptSignals();

  const auto options_result = qtrade::common::process_boot::ParseProgramOptions(argc, argv);
  if (options_result.error_code != qtrade::ErrorCode::kSuccess || !options_result.data.has_value()) {
    std::cerr << "[qtrade_client] Failed to parse program options:" << options_result.error_message << std::endl;
    return EXIT_FAILURE;
  }

  const auto bootstrap_config = qtrade::engine::boot::LoadBootstrapConfig(options_result.data.value());
  if (!bootstrap_config.has_value()) {
    qtrade::common::system::NotifyError(0, "Failed to load engine bootstrap config");
    return EXIT_FAILURE;
  }

  if (!qtrade::common::process_boot::InitProgramEnvironment(qtrade::engine::kServiceName,
                                                            bootstrap_config->config.log_dir,
                                                            bootstrap_config->config.log_filename,
                                                            options_result.data.value())) {
    qtrade::common::system::NotifyError(0, "Failed to initialize program environment");
    return EXIT_FAILURE;
  }

  auto engine = qtrade::engine::CreateEngine();

  std::unique_ptr<qtrade::bridge::GrpcConfigBridge> config_bridge;
  std::unique_ptr<qtrade::bridge::GrpcAccountBridge> account_bridge;
  std::unique_ptr<qtrade::bridge::GrpcAccountRiskBridge> account_risk_bridge;

  if (bootstrap_config->support_services.config_service.enabled) {
    config_bridge = std::make_unique<qtrade::bridge::GrpcConfigBridge>(
      bootstrap_config->support_services.config_service, bootstrap_config->config.identity.engine_id);
    if (config_bridge->Init() != qtrade::ErrorCode::kSuccess) {
      qtrade::common::system::NotifyError(0, "Failed to init config bridge");
      return EXIT_FAILURE;
    }
    engine->SetConfigBridge(config_bridge.get());
  }
  if (bootstrap_config->support_services.account_service.enabled) {
    account_bridge =
      std::make_unique<qtrade::bridge::GrpcAccountBridge>(bootstrap_config->support_services.account_service);
    if (account_bridge->Init() != qtrade::ErrorCode::kSuccess) {
      qtrade::common::system::NotifyError(0, "Failed to init account bridge");
      return EXIT_FAILURE;
    }
    engine->SetAccountBridge(account_bridge.get());
  }
  if (bootstrap_config->support_services.account_risk_service.enabled) {
    account_risk_bridge = std::make_unique<qtrade::bridge::GrpcAccountRiskBridge>(
      bootstrap_config->support_services.account_risk_service);
    if (account_risk_bridge->Init() != qtrade::ErrorCode::kSuccess) {
      qtrade::common::system::NotifyError(0, "Failed to init account risk bridge");
      return EXIT_FAILURE;
    }
    engine->SetAccountRiskBridge(account_risk_bridge.get());
  }

  AdapterBundle adapters;
  if (!BuildAdapters(*bootstrap_config, config_bridge.get(), account_bridge.get(), &adapters)) {
    qtrade::common::system::NotifyError(0, "Failed to build/connect quote/trader adapters");
    return EXIT_FAILURE;
  }
  engine->SetQuoteApi(std::move(adapters.quote_api));
  engine->SetTraderApi(std::move(adapters.trader_api));

  if (engine->Init(bootstrap_config.value()) != qtrade::ErrorCode::kSuccess) {
    qtrade::common::system::NotifyError(0, "Failed to initialize engine");
    return EXIT_FAILURE;
  }

  if (!qtrade::engine::boot::LoadStrategies(*engine, bootstrap_config->config.strategy.plugin_dir)) {
    qtrade::common::system::NotifyError(0, "Failed to load strategies from runtime config");
    return EXIT_FAILURE;
  }

  if (!qtrade::engine::boot::StartEngine(*engine)) {
    qtrade::common::system::NotifyError(0, "Failed to start engine");
    return EXIT_FAILURE;
  }
  (void)qtrade::common::system::NotifyReady("qtrade_client ready");

  qtrade::engine::boot::RunUntilShutdown(*engine);

  if (account_risk_bridge) {
    account_risk_bridge->Shutdown();
  }
  if (account_bridge) {
    account_bridge->Shutdown();
  }
  if (config_bridge) {
    config_bridge->Shutdown();
  }

  qtrade::common::process_boot::LogProcessStopped(qtrade::engine::kServiceName);
  return EXIT_SUCCESS;
}
