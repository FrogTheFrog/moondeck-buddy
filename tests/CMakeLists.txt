find_package(Qt6 COMPONENTS Core REQUIRED)
add_executable(SteamProcessLogTests steamprocesslog.cpp)
target_link_libraries(SteamProcessLogTests PRIVATE steamlib commonlib Qt6::Core)
add_test(NAME SteamProcessLogTests COMMAND SteamProcessLogTests)
