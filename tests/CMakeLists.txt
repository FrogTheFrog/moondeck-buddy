find_package(Qt6 COMPONENTS Core REQUIRED)
add_executable(NonSteamTargetTests nonsteamtarget.cpp)
target_link_libraries(NonSteamTargetTests PRIVATE steamlib Qt6::Core)
add_test(NAME NonSteamTargetTests COMMAND NonSteamTargetTests)
