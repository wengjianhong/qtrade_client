# ---------------------------------------------------------------------------
# Production trading engine client binary + vendor adapters
# ---------------------------------------------------------------------------

file(GLOB_RECURSE QTRADE_CLIENT_ADAPTER_SRC CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/adapters/*.cpp)

add_library(qtrade_client_adapters STATIC ${QTRADE_CLIENT_ADAPTER_SRC})
target_include_directories(qtrade_client_adapters PUBLIC
  ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(qtrade_client_adapters PUBLIC
  qtrade_engine::qtrade_engine
  qtrade_service::qtrade_service_bridges
)

add_executable(qtrade_engine ${CMAKE_SOURCE_DIR}/src/main.cpp)
target_link_libraries(qtrade_engine PRIVATE
  qtrade_client_adapters
)
