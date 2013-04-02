project(fetchwebapp)

cmake_minimum_required(VERSION 2.8)

find_package(Qt4 REQUIRED)
find_package(Automoc4 REQUIRED)
find_package(QJSON REQUIRED)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

automoc4_add_executable(fetchwebapp fetchwebapp.cpp main.cpp)
target_link_libraries(fetchwebapp ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${QT_QTSQL_LIBRARY} ${QJSON_LIBRARIES} ${qjson_LIBRARIES})

install(TARGETS fetchwebapp RUNTIME DESTINATION bin)
