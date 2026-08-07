# ---------------------------------------------------------------------------
# Production trading engine client (qtrade_engine)
# ---------------------------------------------------------------------------

add_executable(qtrade_engine ${CMAKE_SOURCE_DIR}/src/main.cpp)

target_link_libraries(qtrade_engine PRIVATE
  qtrade::qtrade_core
  qtrade_service::qtrade_service_bridges
)
