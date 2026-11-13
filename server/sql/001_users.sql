-- 一期用户表（与 UserRecord 字段对应）
CREATE SEQUENCE IF NOT EXISTS users_id_seq START WITH 10001;

CREATE TABLE IF NOT EXISTS users (
    id              BIGINT PRIMARY KEY DEFAULT nextval('users_id_seq'),
    nickname        VARCHAR(32) NOT NULL,
    password_hash   TEXT NOT NULL,
    salt            TEXT NOT NULL,
    avatar          TEXT NOT NULL DEFAULT '',
    avatar_preset   VARCHAR(32) NOT NULL DEFAULT 'avatar_2',
    total_games     INT NOT NULL DEFAULT 0 CHECK (total_games >= 0),
    wins            INT NOT NULL DEFAULT 0 CHECK (wins >= 0),
    losses          INT NOT NULL DEFAULT 0 CHECK (losses >= 0),
    status          SMALLINT NOT NULL DEFAULT 0,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT users_nickname_unique UNIQUE (nickname)
);
