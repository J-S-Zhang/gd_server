#include "user/postgres_user_repository.h"
#include "utils/logger.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

#ifdef GUANDAN_HAS_POSTGRES
#include <libpq-fe.h>
#endif

/// @file postgres_user_repository.cpp
/// @brief 用户 PostgreSQL 持久化

namespace guandan {

namespace {

std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

}  // namespace

std::string PostgresUserRepository::trimNickname(const std::string& nickname) {
    return trim(nickname);
}

PostgresUserRepository::PostgresUserRepository(std::string connectionUrl)
    : connectionUrl_(std::move(connectionUrl)) {
#ifndef GUANDAN_HAS_POSTGRES
    Logger::error("PostgresUserRepository 需要 GUANDAN_HAS_POSTGRES");
#else
    if (!ensureConnected()) {
        throw std::runtime_error("无法连接 PostgreSQL: " + connectionUrl_);
    }
    auto* conn = static_cast<PGconn*>(conn_);
    PGresult* res = PQexec(conn, "SELECT COUNT(*)::text FROM users");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        Logger::info("PostgreSQL 用户库已连接，users 行数: " +
                     std::string(PQgetvalue(res, 0, 0)));
    } else {
        Logger::warn("PostgreSQL users 表查询失败，请确认已执行 server/sql/001_users.sql");
    }
    PQclear(res);
#endif
}

PostgresUserRepository::~PostgresUserRepository() {
    disconnect();
}

bool PostgresUserRepository::ensureConnected() const {
#ifndef GUANDAN_HAS_POSTGRES
    (void)connectionUrl_;
    return false;
#else
    auto* conn = static_cast<PGconn*>(conn_);
    if (conn != nullptr && PQstatus(conn) == CONNECTION_OK) {
        return true;
    }
    if (conn != nullptr) {
        PQreset(conn);
        if (PQstatus(conn) == CONNECTION_OK) return true;
        PQfinish(conn);
        conn_ = nullptr;
    }
    conn = PQconnectdb(connectionUrl_.c_str());
    conn_ = conn;
    if (conn == nullptr || PQstatus(conn) != CONNECTION_OK) {
        Logger::error("PQconnectdb: " + (conn ? std::string(PQerrorMessage(conn)) : "null"));
        if (conn) {
            PQfinish(conn);
            conn_ = nullptr;
        }
        return false;
    }
    return true;
#endif
}

void PostgresUserRepository::disconnect() const {
#ifdef GUANDAN_HAS_POSTGRES
    if (conn_ != nullptr) {
        PQfinish(static_cast<PGconn*>(conn_));
        conn_ = nullptr;
    }
#endif
}

#ifdef GUANDAN_HAS_POSTGRES
namespace {

std::optional<UserRecord> userFromPgRow(PGresult* res, int row) {
    if (PQntuples(res) <= row || PQnfields(res) < 9) return std::nullopt;
    UserRecord user;
    try {
        user.id = static_cast<PlayerId>(std::stoull(PQgetvalue(res, row, 0)));
        user.nickname = PQgetvalue(res, row, 1);
        user.passwordHash = PQgetvalue(res, row, 2);
        user.salt = PQgetvalue(res, row, 3);
        user.avatar = PQgetvalue(res, row, 4);
        user.avatarPreset = PQgetvalue(res, row, 5);
        user.stats.totalGames = static_cast<uint32_t>(std::stoul(PQgetvalue(res, row, 6)));
        user.stats.wins = static_cast<uint32_t>(std::stoul(PQgetvalue(res, row, 7)));
        user.stats.losses = static_cast<uint32_t>(std::stoul(PQgetvalue(res, row, 8)));
    } catch (...) {
        return std::nullopt;
    }
    return user;
}

}  // namespace
#endif

std::optional<UserRecord> PostgresUserRepository::findByNickname(const std::string& nickname) const {
#ifndef GUANDAN_HAS_POSTGRES
    (void)nickname;
    return std::nullopt;
#else
    std::lock_guard lock(mutex_);
    if (!ensureConnected()) return std::nullopt;

    const std::string nick = trimNickname(nickname);
    const char* params[1] = {nick.c_str()};
    const int lengths[1] = {static_cast<int>(nick.size())};
    const int formats[1] = {0};

    auto* conn = static_cast<PGconn*>(conn_);
    PGresult* res = PQexecParams(
        conn,
        "SELECT id, nickname, password_hash, salt, avatar, avatar_preset, "
        "total_games, wins, losses "
        "FROM users WHERE nickname = $1 AND status = 0",
        1, nullptr, params, lengths, formats, 0);

    std::optional<UserRecord> out;
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) == 1) {
        out = userFromPgRow(res, 0);
    } else if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        Logger::error("findByNickname: " + std::string(PQerrorMessage(conn)));
    }
    PQclear(res);
    return out;
#endif
}

std::optional<UserRecord> PostgresUserRepository::findById(PlayerId id) const {
#ifndef GUANDAN_HAS_POSTGRES
    (void)id;
    return std::nullopt;
#else
    std::lock_guard lock(mutex_);
    if (!ensureConnected()) return std::nullopt;

    const std::string idStr = std::to_string(id);
    const char* params[1] = {idStr.c_str()};
    const int lengths[1] = {static_cast<int>(idStr.size())};
    const int formats[1] = {0};

    auto* conn = static_cast<PGconn*>(conn_);
    PGresult* res = PQexecParams(
        conn,
        "SELECT id, nickname, password_hash, salt, avatar, avatar_preset, "
        "total_games, wins, losses "
        "FROM users WHERE id = $1::bigint AND status = 0",
        1, nullptr, params, lengths, formats, 0);

    std::optional<UserRecord> out;
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) == 1) {
        out = userFromPgRow(res, 0);
    } else if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        Logger::error("findById: " + std::string(PQerrorMessage(conn)));
    }
    PQclear(res);
    return out;
#endif
}

std::string PostgresUserRepository::registerUser(const std::string& nickname,
                                                 const std::string& /*password*/,
                                                 const std::string& passwordHash,
                                                 const std::string& salt) {
#ifndef GUANDAN_HAS_POSTGRES
    (void)nickname;
    (void)passwordHash;
    (void)salt;
    return "服务器未启用 PostgreSQL 支持";
#else
    std::lock_guard lock(mutex_);
    if (!ensureConnected()) return "数据库连接失败，请稍后重试";

    const std::string nick = trimNickname(nickname);
    if (nick.size() < 2 || nick.size() > 16) {
        return "昵称长度需在 2-16 个字符之间";
    }

    const char* params[3] = {nick.c_str(), passwordHash.c_str(), salt.c_str()};
    const int lengths[3] = {static_cast<int>(nick.size()), static_cast<int>(passwordHash.size()),
                            static_cast<int>(salt.size())};
    const int formats[3] = {0, 0, 0};

    auto* conn = static_cast<PGconn*>(conn_);
    PGresult* res = PQexecParams(
        conn,
        "INSERT INTO users (nickname, password_hash, salt, avatar_preset) "
        "VALUES ($1, $2, $3, 'avatar_2') RETURNING id",
        3, nullptr, params, lengths, formats, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        const char* sqlState = PQresultErrorField(res, PG_DIAG_SQLSTATE);
        std::string err = PQerrorMessage(conn);
        PQclear(res);
        if (sqlState && std::strcmp(sqlState, "23505") == 0) {
            return "昵称已被使用，请更换昵称";
        }
        Logger::error("registerUser: " + err);
        return "注册失败，请稍后重试";
    }
    PQclear(res);
    return "";
#endif
}

bool PostgresUserRepository::updateStats(PlayerId id, const UserStats& stats) {
#ifndef GUANDAN_HAS_POSTGRES
    (void)id;
    (void)stats;
    return false;
#else
    std::lock_guard lock(mutex_);
    if (!ensureConnected()) return false;

    const std::string idStr = std::to_string(id);
    const std::string tg = std::to_string(stats.totalGames);
    const std::string w = std::to_string(stats.wins);
    const std::string l = std::to_string(stats.losses);
    const char* params[4] = {idStr.c_str(), tg.c_str(), w.c_str(), l.c_str()};
    const int lengths[4] = {static_cast<int>(idStr.size()), static_cast<int>(tg.size()),
                            static_cast<int>(w.size()), static_cast<int>(l.size())};
    const int formats[4] = {0, 0, 0, 0};

    auto* conn = static_cast<PGconn*>(conn_);
    PGresult* res = PQexecParams(
        conn,
        "UPDATE users SET total_games = $2::int, wins = $3::int, losses = $4::int, "
        "updated_at = now() WHERE id = $1::bigint",
        4, nullptr, params, lengths, formats, 0);

    const bool ok =
        PQresultStatus(res) == PGRES_COMMAND_OK && std::strcmp(PQcmdTuples(res), "0") != 0;
    if (!ok && PQresultStatus(res) != PGRES_COMMAND_OK) {
        Logger::error("updateStats: " + std::string(PQerrorMessage(conn)));
    }
    PQclear(res);
    return ok;
#endif
}

bool PostgresUserRepository::updateAvatar(PlayerId id, const std::string& avatarPath) {
#ifndef GUANDAN_HAS_POSTGRES
    (void)id;
    (void)avatarPath;
    return false;
#else
    std::lock_guard lock(mutex_);
    if (!ensureConnected()) return false;

    const std::string idStr = std::to_string(id);
    const char* params[2] = {idStr.c_str(), avatarPath.c_str()};
    const int lengths[2] = {static_cast<int>(idStr.size()), static_cast<int>(avatarPath.size())};
    const int formats[2] = {0, 0};

    auto* conn = static_cast<PGconn*>(conn_);
    PGresult* res = PQexecParams(
        conn,
        "UPDATE users SET avatar = $2, updated_at = now() WHERE id = $1::bigint",
        2, nullptr, params, lengths, formats, 0);

    const bool ok =
        PQresultStatus(res) == PGRES_COMMAND_OK && std::strcmp(PQcmdTuples(res), "0") != 0;
    if (!ok && PQresultStatus(res) != PGRES_COMMAND_OK) {
        Logger::error("updateAvatar: " + std::string(PQerrorMessage(conn)));
    }
    PQclear(res);
    return ok;
#endif
}

bool PostgresUserRepository::updateAvatarPreset(PlayerId id, const std::string& presetId) {
#ifndef GUANDAN_HAS_POSTGRES
    (void)id;
    (void)presetId;
    return false;
#else
    std::lock_guard lock(mutex_);
    if (!ensureConnected()) return false;

    const std::string idStr = std::to_string(id);
    const char* params[2] = {idStr.c_str(), presetId.c_str()};
    const int lengths[2] = {static_cast<int>(idStr.size()), static_cast<int>(presetId.size())};
    const int formats[2] = {0, 0};

    auto* conn = static_cast<PGconn*>(conn_);
    PGresult* res = PQexecParams(
        conn,
        "UPDATE users SET avatar_preset = $2, updated_at = now() WHERE id = $1::bigint",
        2, nullptr, params, lengths, formats, 0);

    const bool ok =
        PQresultStatus(res) == PGRES_COMMAND_OK && std::strcmp(PQcmdTuples(res), "0") != 0;
    if (!ok && PQresultStatus(res) != PGRES_COMMAND_OK) {
        Logger::error("updateAvatarPreset: " + std::string(PQerrorMessage(conn)));
    }
    PQclear(res);
    return ok;
#endif
}

}  // namespace guandan
