# qtrade_client

量化交易客户端：组合已安装的 `qtrade_common`、`qtrade_engine`（核心库 / `IEngine`）与 `qtrade_service`（proto + 微服务），产出进程 **`qtrade_client`**。

本仓自行维护并编译：

- `src/adapters/`：行情/交易适配器（mock、emt）
- `src/client/`：支撑服务 gRPC 薄客户端
- `src/bridge/`：`Grpc*Bridge`（注入引擎前由本进程 Init）

**不**实现引擎核心或微服务；策略 `.so` 由 `qtrade_strategy` 提供。

## 构建顺序

```bash
# 1. cpputils
# 2. qtrade_common
# 3. qtrade_engine（库）
# 4. qtrade_service（微服务 + proto 库）
# 5. qtrade_strategy（可选，策略 so）
# 6. 本仓

cmake -S . -B build \
  -DCMAKE_INSTALL_PREFIX=/usr/local/qtrade \
  -DCMAKE_PREFIX_PATH="/usr/local/cpputils;/usr/local/qtrade"
cmake --build build -j1
sudo cmake --install build
```

## 运行

```bash
export PATH=/usr/local/qtrade/bin:$PATH
qtrade_client --config /usr/local/qtrade/config/qtrade_engine.json
```

引导配置中 `config.strategy.plugin_dir` 应指向策略安装目录，例如 `/usr/local/qtrade/lib/strategies`。
