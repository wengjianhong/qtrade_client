# qtrade_client

量化交易客户端进程：组合已安装的 `qtrade_common`、`qtrade_engine`（`IEngine`）与 `qtrade_service`（proto），产出 **`qtrade_client`**。

本仓自行维护：

- `src/adapters/`：行情/交易适配器（mock、emt）
- `src/client/`：支撑服务 gRPC 薄客户端
- `src/bridge/`：`Grpc*Bridge`（注入引擎前由本进程 Init）

**不**实现引擎核心或微服务；策略 `.so` 由 **qtrade_strategy** 提供。

## 项目关系

```text
启动后进程关系：

  qtrade_config_service :50051
  qtrade_account_service :50052
  qtrade_account_risk_service :50053
           ↑ gRPC（本仓 bridge/client）
  qtrade_client  ──CreateEngine()──► libqtrade_engine.so
       │
       ├── dlopen  qtrade_strategy 安装的 .so
       └── QuoteApi / TraderApi（本仓 adapters：mock 或 emt）
```

构建依赖顺序：cpputils → common → engine → service → strategy（可选）→ **本仓**。

## 环境要求

- C++20 及以上
- CMake 3.22+
- 已安装 cpputils（`/usr/local/cpputils`）以及同前缀下的 qtrade_common、qtrade_engine、qtrade_service
- 操作系统：Linux（推荐 Ubuntu 24.04+）

## 编译安装

```shell
cmake -B build \
  -DCMAKE_INSTALL_PREFIX=/usr/local/qtrade \
  -DCMAKE_PREFIX_PATH="/usr/local/cpputils;/usr/local/qtrade"
cmake --build build -j1
sudo cmake --install build
```

安装布局：

```
/usr/local/qtrade/
├── bin/qtrade_client
└── config/qtrade_engine.json
```

全链安装与编排见并列仓 **qtrade_devops**。

## 运行

1. MySQL 已按各服务 json 建库（见 **qtrade_service**）
2. 先启动三个支撑服务
3. 策略已安装到 `plugin_dir`（默认 `/usr/local/qtrade/lib/strategies`）
4. 启动客户端：

```shell
export PATH=/usr/local/qtrade/bin:$PATH
qtrade_client --config /usr/local/qtrade/config/qtrade_engine.json
```

未安装时：在本仓根目录 `./build/bin/qtrade_client --config config/qtrade_engine.json`。Ctrl+C 停机。

引导配置要点：

- `support_services`：须能连上 config / account / account-risk 的 host 与端口
- `config.strategy.plugin_dir`：策略 `.so` 目录
- `config.identity`：`engine_id` / `account_id`

无远端引擎配置时，适配器默认走 **mock**（本地开发）。生产/联调柜台时由 config-service 下发适配器类型（如 `emt`）及连接串。
