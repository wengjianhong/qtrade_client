# qtrade_client

企业级量化交易客户端参考实现：组合已安装的 `qtrade`（`IEngine`）与 `qtrade_service`（`Grpc*Bridge`），产出生产进程 `qtrade_engine`。

本仓**不**实现引擎核心或支撑微服务；策略 `.so` 由 `qtrade_strategy` 提供。

## 构建顺序

```bash
# 1. qtrade（库）
# 2. qtrade_service（服务 + bridges 库）
# 3. qtrade_strategy（可选，策略 so）
# 4. 本仓

cmake -B build \
  -DCMAKE_INSTALL_PREFIX=/usr/local/qtrade \
  -DCMAKE_PREFIX_PATH="/usr/local/cpputils;/usr/local/qtrade"
cmake --build build -j1
sudo cmake --install build
```

## 运行

```bash
export PATH=/usr/local/qtrade/bin:$PATH
qtrade_engine --config /usr/local/qtrade/config/qtrade_engine.json
```

引导配置中 `config.strategy.plugin_dir` 应指向策略安装目录，例如 `/usr/local/qtrade/lib/strategies`。
