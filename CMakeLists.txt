cmake_minimum_required(VERSION 3.10)
project(portfolio-management-system)

set(CMAKE_CXX_STANDARD 17)


find_package(Boost REQUIRED COMPONENTS system)
find_package(CURL REQUIRED)


include_directories(${CMAKE_SOURCE_DIR}/dependency/Crow/include)

add_executable(portfolio-management-system main.cpp)

target_link_libraries(portfolio-management-system Boost::boost)
target_link_libraries(portfolio-management-system boost_system)
target_link_libraries(portfolio-management-system pthread)
include_directories(${CURL_INCLUDE_DIRS})
target_link_libraries(portfolio-management-system ${CURL_LIBRARIES})
# target_link_libraries(portfolio-management-system PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(portfolio-management-system 
    Boost::system
    pthread
)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)