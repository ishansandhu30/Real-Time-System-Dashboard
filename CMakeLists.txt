cmake_minimum_required(VERSION 3.16)
project(RealTimeSystemDashboard VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 first, fall back to Qt5
find_package(Qt6 COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 5.12 REQUIRED COMPONENTS Core Gui Widgets OpenGL)
    message(STATUS "Using Qt5: ${Qt5_VERSION}")
    set(QT_LIBS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::OpenGL)
else()
    message(STATUS "Using Qt6: ${Qt6_VERSION}")
    set(QT_LIBS Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)
endif()

find_package(OpenGL REQUIRED)

set(DATA_SOURCES
    src/data/system_data_collector.cpp
)

set(RENDERING_SOURCES
    src/rendering/gl_renderer.cpp
    src/rendering/line_graph.cpp
    src/rendering/bar_chart.cpp
)

set(WIDGET_SOURCES
    src/widgets/gl_chart_widget.cpp
    src/widgets/cpu_monitor.cpp
    src/widgets/memory_monitor.cpp
    src/widgets/process_table.cpp
)

set(APP_SOURCES
    src/app/dashboard.cpp
    src/app/settings_dialog.cpp
)

set(ALL_SOURCES
    src/main.cpp
    ${DATA_SOURCES}
    ${RENDERING_SOURCES}
    ${WIDGET_SOURCES}
    ${APP_SOURCES}
)

set(ALL_HEADERS
    src/data/metrics.h
    src/data/data_buffer.h
    src/data/system_data_collector.h
    src/rendering/gl_renderer.h
    src/rendering/line_graph.h
    src/rendering/bar_chart.h
    src/widgets/gl_chart_widget.h
    src/widgets/cpu_monitor.h
    src/widgets/memory_monitor.h
    src/widgets/process_table.h
    src/app/dashboard.h
    src/app/settings_dialog.h
)

add_executable(${PROJECT_NAME} ${ALL_SOURCES} ${ALL_HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/data
    ${CMAKE_SOURCE_DIR}/src/rendering
    ${CMAKE_SOURCE_DIR}/src/widgets
    ${CMAKE_SOURCE_DIR}/src/app
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_LIBS}
    OpenGL::GL
)

# Copy config and resources to build directory
file(COPY ${CMAKE_SOURCE_DIR}/config DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})

# Install rules
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY config/ DESTINATION share/${PROJECT_NAME}/config)
install(DIRECTORY resources/ DESTINATION share/${PROJECT_NAME}/resources)
