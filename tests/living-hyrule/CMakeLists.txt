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

add_executable(living_hyrule_population_tests PopulationTests.cpp)
target_compile_features(living_hyrule_population_tests PRIVATE cxx_std_17)
target_include_directories(living_hyrule_population_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../soh/soh/Enhancements/living-hyrule"
)
if(MSVC)
    target_compile_options(living_hyrule_population_tests PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(living_hyrule_population_tests PRIVATE -Wall -Wextra -Werror -pedantic)
endif()
add_test(NAME living_hyrule_population_tests COMMAND living_hyrule_population_tests)

add_executable(living_hyrule_property_tests PropertyTests.cpp)
target_compile_features(living_hyrule_property_tests PRIVATE cxx_std_17)
target_include_directories(living_hyrule_property_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../soh/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../soh/soh/Enhancements/living-hyrule"
)
if(MSVC)
    target_compile_options(living_hyrule_property_tests PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(living_hyrule_property_tests PRIVATE -Wall -Wextra -Werror -pedantic)
endif()
add_test(NAME living_hyrule_property_tests COMMAND living_hyrule_property_tests)

add_executable(living_hyrule_trade_tests TradeTests.cpp)
target_compile_features(living_hyrule_trade_tests PRIVATE cxx_std_17)
target_include_directories(living_hyrule_trade_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../soh/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../soh/soh/Enhancements/living-hyrule"
)
if(MSVC)
    target_compile_options(living_hyrule_trade_tests PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(living_hyrule_trade_tests PRIVATE -Wall -Wextra -Werror -pedantic)
endif()
add_test(NAME living_hyrule_trade_tests COMMAND living_hyrule_trade_tests)

add_executable(living_hyrule_challenge_tests ChallengeTests.cpp)
target_compile_features(living_hyrule_challenge_tests PRIVATE cxx_std_17)
target_include_directories(living_hyrule_challenge_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../soh/soh/Enhancements/living-hyrule"
)
if(MSVC)
    target_compile_options(living_hyrule_challenge_tests PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(living_hyrule_challenge_tests PRIVATE -Wall -Wextra -Werror -pedantic)
endif()
add_test(NAME living_hyrule_challenge_tests COMMAND living_hyrule_challenge_tests)

add_executable(living_hyrule_world_population_tests WorldPopulationTests.cpp)
target_compile_features(living_hyrule_world_population_tests PRIVATE cxx_std_17)
target_include_directories(living_hyrule_world_population_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../soh/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../soh/soh/Enhancements/living-hyrule"
)
if(MSVC)
    target_compile_options(living_hyrule_world_population_tests PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(living_hyrule_world_population_tests PRIVATE -Wall -Wextra -Werror -pedantic)
endif()
add_test(NAME living_hyrule_world_population_tests COMMAND living_hyrule_world_population_tests)

foreach(suite IN ITEMS Scenery ForestMountain WaterDesert Social RoyalAudience MarketRestoration)
    string(TOLOWER "${suite}" suite_lower)
    set(target_name "living_hyrule_${suite_lower}_tests")
    add_executable(${target_name} "${suite}Tests.cpp")
    target_compile_features(${target_name} PRIVATE cxx_std_17)
    target_include_directories(${target_name} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../../soh/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../soh/soh/Enhancements/living-hyrule"
    )
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /WX /permissive-)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Werror -pedantic)
    endif()
    add_test(NAME ${target_name} COMMAND ${target_name})
endforeach()

# Optional read-only checks against the owner's existing local archive. CI and
# source-only checkouts still run every pure suite without any Nintendo assets.
set(LIVING_HYRULE_NATIVE_ARCHIVE "" CACHE FILEPATH "Local oot.o2r for offline resource checks")
if(LIVING_HYRULE_NATIVE_ARCHIVE)
    if(NOT EXISTS "${LIVING_HYRULE_NATIVE_ARCHIVE}")
        message(FATAL_ERROR "The selected local native archive does not exist")
    endif()
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME living_hyrule_market_resources
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/MarketRestorationResourceTests.py"
                "${LIVING_HYRULE_NATIVE_ARCHIVE}")
endif()
