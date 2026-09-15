cmake_minimum_required(VERSION 3.16)
project(LivingHyruleEconomyTests LANGUAGES CXX)

enable_testing()
add_executable(living_hyrule_economy_tests EconomyTests.cpp)
target_compile_features(living_hyrule_economy_tests PRIVATE cxx_std_17)
target_include_directories(living_hyrule_economy_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../soh/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../soh/soh/Enhancements/living-hyrule"
)
if(MSVC)
    target_compile_options(living_hyrule_economy_tests PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(living_hyrule_economy_tests PRIVATE -Wall -Wextra -Werror -pedantic)
endif()
add_test(NAME living_hyrule_economy_tests COMMAND living_hyrule_economy_tests)

find_path(LIVING_HYRULE_JSON_INCLUDE nlohmann/json.hpp REQUIRED)
add_executable(living_hyrule_save_tests SaveCodecTests.cpp)
target_compile_features(living_hyrule_save_tests PRIVATE cxx_std_17)
target_include_directories(living_hyrule_save_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../soh"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../soh/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../soh/soh/Enhancements/living-hyrule"
    "${LIVING_HYRULE_JSON_INCLUDE}"
)
if(MSVC)
    target_compile_options(living_hyrule_save_tests PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(living_hyrule_save_tests PRIVATE -Wall -Wextra -Werror -pedantic)
endif()
add_test(NAME living_hyrule_save_tests COMMAND living_hyrule_save_tests)
