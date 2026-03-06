find_package(Qt6 QUIET COMPONENTS Core Widgets Network Multimedia MultimediaWidgets)

if(NOT Qt6_FOUND)
  message(WARNING "Qt6 not found, skip building qt_client")
  return()
endif()

add_executable(qt_client
  app/main.cpp
  app/main_window.cpp
  network/tcp_client_worker.cpp
  media/stream_worker.cpp
  media/record_worker.cpp
)

target_include_directories(qt_client PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(qt_client PRIVATE
  project_common
  Qt6::Core Qt6::Widgets Qt6::Network Qt6::Multimedia Qt6::MultimediaWidgets
)
