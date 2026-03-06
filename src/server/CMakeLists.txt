add_executable(sim_server
  app/main.cpp
  network/tcp_sim_server.cpp
  media/rtsp_launcher.cpp
)

target_include_directories(sim_server PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(sim_server PRIVATE project_common)
