# ---------------------------------------------------------------------------
# Production trading client: adapters + gRPC service clients/bridges + binary
#
# Bridges / thin gRPC clients live in THIS repo and are compiled locally
# (not installed as libs). qtrade_service only supplies proto (+ microservices).
# ---------------------------------------------------------------------------

# Build overlay so #include "qtrade/bridge/...", "qtrade/client/...", 
# "qtrade/common/proto/strategy_config_utils.hpp" resolve to local sources
# without colliding with installed I*Bridge headers under include/qtrade/bridge/.
set(QTRADE_CLIENT_INCLUDE_OVERLAY_DIR ${CMAKE_BINARY_DIR}/include_overlay)
file(MAKE_DIRECTORY ${QTRADE_CLIENT_INCLUDE_OVERLAY_DIR}/qtrade)
file(MAKE_DIRECTORY ${QTRADE_CLIENT_INCLUDE_OVERLAY_DIR}/qtrade/common)

set(_qc_ov_bridge "${QTRADE_CLIENT_INCLUDE_OVERLAY_DIR}/qtrade/bridge")
set(_qc_ov_client "${QTRADE_CLIENT_INCLUDE_OVERLAY_DIR}/qtrade/client")
set(_qc_ov_proto "${QTRADE_CLIENT_INCLUDE_OVERLAY_DIR}/qtrade/common/proto")
foreach(_link IN ITEMS ${_qc_ov_bridge} ${_qc_ov_client} ${_qc_ov_proto})
  if(EXISTS "${_link}" OR IS_SYMLINK "${_link}")
    file(REMOVE "${_link}")
  endif()
endforeach()
file(CREATE_LINK "${CMAKE_SOURCE_DIR}/src/bridge" "${_qc_ov_bridge}" SYMBOLIC)
file(CREATE_LINK "${CMAKE_SOURCE_DIR}/src/client" "${_qc_ov_client}" SYMBOLIC)
file(CREATE_LINK "${CMAKE_SOURCE_DIR}/src/common/proto" "${_qc_ov_proto}" SYMBOLIC)
unset(_qc_ov_bridge)
unset(_qc_ov_client)
unset(_qc_ov_proto)

# --- gRPC thin clients (local static; not installed) ---
file(GLOB_RECURSE QTRADE_CLIENT_SERVICE_CLIENT_SRC CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/client/*.cpp)
add_library(qtrade_client_service_client STATIC ${QTRADE_CLIENT_SERVICE_CLIENT_SRC})
target_include_directories(qtrade_client_service_client PUBLIC
  ${QTRADE_CLIENT_INCLUDE_OVERLAY_DIR}
  ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(qtrade_client_service_client PUBLIC
  qtrade_engine::qtrade_engine
  qtrade_service::qtrade_service_proto
  spdlog::spdlog
)

# --- Grpc*Bridge + strategy_config_utils (local static; not installed) ---
file(GLOB_RECURSE QTRADE_CLIENT_BRIDGE_SRC CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/bridge/*.cpp)
add_library(qtrade_client_bridges STATIC
  ${QTRADE_CLIENT_BRIDGE_SRC}
  ${CMAKE_SOURCE_DIR}/src/common/proto/strategy_config_utils.cpp
)
target_include_directories(qtrade_client_bridges PUBLIC
  ${QTRADE_CLIENT_INCLUDE_OVERLAY_DIR}
  ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(qtrade_client_bridges PUBLIC
  qtrade_client_service_client
)

# --- Vendor adapters ---
file(GLOB_RECURSE QTRADE_CLIENT_ADAPTER_SRC CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/adapters/*.cpp)
add_library(qtrade_client_adapters STATIC ${QTRADE_CLIENT_ADAPTER_SRC})
target_include_directories(qtrade_client_adapters PUBLIC
  ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(qtrade_client_adapters PUBLIC
  qtrade_engine::qtrade_engine
)

file(GLOB_RECURSE QTRADE_CLIENT_BOOTSTRAP_SRC CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/bootstrap/*.cpp)
add_library(qtrade_client_bootstrap STATIC ${QTRADE_CLIENT_BOOTSTRAP_SRC})
target_include_directories(qtrade_client_bootstrap PUBLIC
  ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(qtrade_client_bootstrap PUBLIC
  qtrade_common::qtrade_common
  qtrade_engine::qtrade_engine
)

add_executable(qtrade_client ${CMAKE_SOURCE_DIR}/src/main.cpp)
target_include_directories(qtrade_client PRIVATE
  ${QTRADE_CLIENT_INCLUDE_OVERLAY_DIR}
)
target_link_libraries(qtrade_client PRIVATE
  qtrade_client_adapters
  qtrade_client_bridges
  qtrade_client_bootstrap
)
