cmake_minimum_required(VERSION 3.16)
project(guandan_server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    set(PLATFORM_LIBS ws2_32)
endif()

set(GUANDAN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(GUANDAN_CORE_SOURCES
    src/game/card.cpp
    src/game/deck.cpp
    src/game/hand.cpp
    src/game/card_analyzer.cpp
    src/game/rule_engine.cpp
    src/game/game_state.cpp
    src/game/turn_manager.cpp
    src/game/settlement.cpp
    src/game/team_progress.cpp
    src/game/tribute.cpp
    src/game/bot_player.cpp
    src/game/game_engine.cpp
    src/room/room.cpp
    src/room/dismiss_vote.cpp
    src/room/room_manager.cpp
    src/room/room_task_queue.cpp
    src/protocol/message.cpp
    src/protocol/game_view_builder.cpp
    src/utils/random.cpp
    src/utils/logger.cpp
    src/utils/uuid.cpp
    src/utils/sha1.cpp
    src/user/user_store.cpp
    src/auth/password_hash.cpp
    src/auth/token_manager.cpp
    src/auth/auth_service.cpp
    src/game/room_config.cpp
)

add_library(guandan_core STATIC ${GUANDAN_CORE_SOURCES})
target_include_directories(guandan_core PUBLIC ${GUANDAN_INCLUDE_DIR})

set(GUANDAN_SERVER_SOURCES
    src/network/websocket_server.cpp
    src/network/http_server.cpp
    src/network/connection_manager.cpp
    src/network/session_manager.cpp
    src/network/message_dispatcher.cpp
    src/timer/timer_manager.cpp
    src/main.cpp
)

add_executable(guandan_server ${GUANDAN_SERVER_SOURCES})
target_link_libraries(guandan_server PRIVATE guandan_core ${PLATFORM_LIBS})

enable_testing()

set(GUANDAN_TEST_SOURCES
    tests/test_main.cpp
    tests/test_card.cpp
    tests/test_card_analyzer.cpp
    tests/test_rule_engine.cpp
    tests/test_game_engine.cpp
    tests/test_turn_manager.cpp
    tests/test_bot_player.cpp
    tests/test_settlement.cpp
    tests/test_pass_a.cpp
    tests/test_tribute.cpp
    tests/test_message.cpp
)

add_executable(guandan_tests ${GUANDAN_TEST_SOURCES})
target_link_libraries(guandan_tests PRIVATE guandan_core)
add_test(NAME guandan_tests COMMAND guandan_tests)

option(GUANDAN_ENABLE_BOOST "Enable Boost.Beast WebSocket (alternative)" OFF)
if(GUANDAN_ENABLE_BOOST)
    find_package(Boost REQUIRED COMPONENTS system)
    target_compile_definitions(guandan_server PRIVATE GUANDAN_USE_BOOST)
    target_link_libraries(guandan_server PRIVATE Boost::system)
endif()
