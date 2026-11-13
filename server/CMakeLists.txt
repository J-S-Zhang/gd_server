cmake_minimum_required(VERSION 3.16)
project(guandan_server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    set(PLATFORM_LIBS ws2_32)
endif()

find_package(Threads REQUIRED)

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
    src/game/finish_rank.cpp
    src/game/team_progress.cpp
    src/game/tribute.cpp
    src/game/bot_player.cpp
    src/game/game_engine.cpp
    src/room/room.cpp
    src/room/dismiss_vote.cpp
    src/room/room_manager.cpp
    src/room/room_task_queue.cpp
    src/room/room_executor.cpp
    src/room/emotion.cpp
    src/protocol/message.cpp
    src/protocol/game_view_builder.cpp
    src/utils/random.cpp
    src/utils/logger.cpp
    src/utils/uuid.cpp
    src/utils/sha1.cpp
    src/utils/hmac_sha256.cpp
    src/livekit/livekit_config.cpp
    src/livekit/livekit_token.cpp
    src/user/json_user_repository.cpp
    src/user/user_repository_factory.cpp
    src/user/avatar_storage.cpp
    src/auth/password_hash.cpp
    src/auth/token_manager.cpp
    src/auth/auth_service.cpp
    src/game/room_config.cpp
)

option(GUANDAN_USE_POSTGRES "Persist users in PostgreSQL when postgres_url is configured" ON)

set(GUANDAN_USER_PG_SOURCES src/user/postgres_user_repository.cpp)
if(GUANDAN_USE_POSTGRES)
    find_package(PostgreSQL QUIET)
    if(PostgreSQL_FOUND)
        list(APPEND GUANDAN_CORE_SOURCES ${GUANDAN_USER_PG_SOURCES})
        message(STATUS "Guandan user storage: PostgreSQL client (libpq) enabled")
    else()
        message(STATUS "Guandan user storage: libpq not found, PostgreSQL backend disabled at build time")
    endif()
endif()

add_library(guandan_core STATIC ${GUANDAN_CORE_SOURCES})
target_include_directories(guandan_core PUBLIC ${GUANDAN_INCLUDE_DIR})

if(GUANDAN_USE_POSTGRES AND PostgreSQL_FOUND)
    target_compile_definitions(guandan_core PUBLIC GUANDAN_HAS_POSTGRES)
    target_include_directories(guandan_core PRIVATE ${PostgreSQL_INCLUDE_DIRS})
    target_link_libraries(guandan_core PUBLIC PostgreSQL::PostgreSQL)
endif()

# -----------------------------------------------------------------------------
# 网络模型开关（编译期二选一）
#   OFF（默认）：原生 socket，一连接一线程  → websocket_server.cpp
#   ON         ：Boost.Asio/Beast I/O 线程池 → websocket_server_asio.cpp
#
# 配置示例：
#   cmake -DGUANDAN_ENABLE_BOOST=ON ..
#   cmake -DGUANDAN_ENABLE_BOOST=OFF ..
# -----------------------------------------------------------------------------
option(GUANDAN_ENABLE_BOOST
    "Use Boost.Asio/Beast WebSocket I/O thread pool instead of native one-thread-per-connection"
    OFF)

set(GUANDAN_SERVER_SOURCES
    src/network/http_server.cpp
    src/network/session_manager.cpp
    src/network/message_dispatcher.cpp
    src/timer/timer_manager.cpp
    src/main.cpp
)

if(GUANDAN_ENABLE_BOOST)
    list(APPEND GUANDAN_SERVER_SOURCES src/network/websocket_server_asio.cpp)
    message(STATUS "Guandan network model: Boost.Asio/Beast I/O thread pool (GUANDAN_ENABLE_BOOST=ON)")
else()
    list(APPEND GUANDAN_SERVER_SOURCES
        src/network/websocket_server.cpp
        src/network/connection_manager.cpp
    )
    message(STATUS "Guandan network model: native socket one-thread-per-connection (GUANDAN_ENABLE_BOOST=OFF)")
endif()

add_executable(guandan_server ${GUANDAN_SERVER_SOURCES})
target_link_libraries(guandan_server PRIVATE guandan_core ${PLATFORM_LIBS} Threads::Threads)

if(GUANDAN_ENABLE_BOOST)
    find_package(Boost REQUIRED COMPONENTS system)
    target_compile_definitions(guandan_server PRIVATE GUANDAN_USE_BOOST)
    target_include_directories(guandan_server PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(guandan_server PRIVATE Boost::system)
endif()

set(GUANDAN_VOICE_SOURCES
    src/voice/voice_room_manager.cpp
    src/voice/voice_server.cpp
    src/voice/main.cpp
    src/utils/logger.cpp
)

add_executable(guandan_voice_server ${GUANDAN_VOICE_SOURCES})
target_include_directories(guandan_voice_server PUBLIC ${GUANDAN_INCLUDE_DIR})
target_link_libraries(guandan_voice_server PRIVATE ${PLATFORM_LIBS})

enable_testing()

set(GUANDAN_TEST_SOURCES
    tests/test_main.cpp
    # 临时只跑多房间并发性能；恢复全量时取消下面注释
    # tests/test_card.cpp
    # tests/test_card_analyzer.cpp
    # tests/test_rule_engine.cpp
    # tests/test_game_engine.cpp
    # tests/test_turn_manager.cpp
    # tests/test_bot_player.cpp
    # tests/test_settlement.cpp
    # tests/test_finish_rank.cpp
    # tests/test_pass_a.cpp
    # tests/test_tribute.cpp
    # tests/test_message.cpp
    tests/test_multi_room_bot_perf.cpp
)

add_executable(guandan_tests ${GUANDAN_TEST_SOURCES})
target_link_libraries(guandan_tests PRIVATE guandan_core)
add_test(NAME guandan_tests COMMAND guandan_tests)
