/// @file      main.cpp
/// @brief     企业级交易引擎客户端入口（qtrade_client）
/// @details   依赖已安装的 qtrade（IEngine）与 qtrade_service（Grpc*Bridge）；
///            创建桥接并注入后 Init / 加载策略 / Start。
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
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

int main(int argc, char** argv) {
  qtrade::common::system::BlockInterruptSignals();

  const auto options_result = qtrade::common::process_boot::ParseProgramOptions(argc, argv);
  if (options_result.error_code != qtrade::ErrorCode::kSuccess || !options_result.data.has_value()) {
    std::cerr << "[qtrade_engine] Failed to parse program options:" << options_result.error_message << std::endl;
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

  // 桥接由本进程持有；Init 内统一调用 I*Bridge::Start()
  std::unique_ptr<qtrade::bridge::GrpcConfigBridge> config_bridge;
  std::unique_ptr<qtrade::bridge::GrpcAccountBridge> account_bridge;
  std::unique_ptr<qtrade::bridge::GrpcAccountRiskBridge> account_risk_bridge;

  if (bootstrap_config->support_services.config_service.enabled) {
    config_bridge = std::make_unique<qtrade::bridge::GrpcConfigBridge>(
      bootstrap_config->support_services.config_service, bootstrap_config->config.identity.engine_id);
    engine->SetConfigBridge(config_bridge.get());
  }
  if (bootstrap_config->support_services.account_service.enabled) {
    account_bridge =
      std::make_unique<qtrade::bridge::GrpcAccountBridge>(bootstrap_config->support_services.account_service);
    engine->SetAccountBridge(account_bridge.get());
  }
  if (bootstrap_config->support_services.account_risk_service.enabled) {
    account_risk_bridge = std::make_unique<qtrade::bridge::GrpcAccountRiskBridge>(
      bootstrap_config->support_services.account_risk_service);
    engine->SetAccountRiskBridge(account_risk_bridge.get());
  }

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
  (void)qtrade::common::system::NotifyReady("qtrade_engine ready");

  qtrade::engine::boot::RunUntilShutdown(*engine);

  qtrade::common::process_boot::LogProcessStopped(qtrade::engine::kServiceName);
  return EXIT_SUCCESS;
}
