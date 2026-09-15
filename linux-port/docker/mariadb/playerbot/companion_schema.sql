-- The companion mod: which Playerbots each player keeps as companions.
--
-- Up to ten rows per owner (the core enforces the number; the table only has to
-- store them). Read once per process and re-read on the top-up cadence, and
-- written when a player runs /companion or whispers a bot, so the choice
-- survives a restart - which is the whole point of keeping it here rather than
-- in memory.
--
-- The key is (owner_pid, companion_pid): one player may keep several bots, and
-- taking the same one twice is the same row. companion_pid stays UNIQUE on its
-- own, so one bot can never be two people's companion. The core checks both as
-- well - a database edited by hand is not allowed to leave the two halves
-- disagreeing - but the constraints are what stop it happening in the first
-- place.
--
-- created_at is not decoration: the core orders by it, and that order is the
-- marching formation. Slot 0 walks directly behind the owner.
--
-- Deliberately NOT a foreign key on player.player: those tables are MyISAM in
-- this world, and a constraint MyISAM accepts and ignores is worse than none.
-- The core validates every row against the seed registry on load instead, and
-- a row naming a character that is not a registered Playerbot is ignored.
--
-- Idempotent: this file is applied on every Compose start.
CREATE TABLE IF NOT EXISTS common.playerbot_companion (
    owner_pid     INT UNSIGNED     NOT NULL,
    companion_pid INT UNSIGNED     NOT NULL,
    enabled       TINYINT UNSIGNED NOT NULL DEFAULT 1,
    created_at    TIMESTAMP        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (owner_pid, companion_pid),
    UNIQUE KEY uq_playerbot_companion_companion (companion_pid)
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4;

-- Migration for a world created by the one-companion version of this file,
-- where the primary key was owner_pid alone. CREATE TABLE IF NOT EXISTS does
-- nothing to a table that already exists, so without this an existing server
-- would keep refusing the second companion at the database level while the core
-- happily allowed ten.
--
-- Conditional on the key really being single-column, so the ALTER runs once and
-- every later start does nothing. Existing rows are kept: the old key already
-- guaranteed one row per owner, so widening it cannot collide.
SET @companion_pk_columns = (
    SELECT COUNT(*)
    FROM information_schema.STATISTICS
    WHERE TABLE_SCHEMA = 'common'
      AND TABLE_NAME = 'playerbot_companion'
      AND INDEX_NAME = 'PRIMARY');

SET @companion_migrate = IF(@companion_pk_columns = 1,
    'ALTER TABLE common.playerbot_companion
        DROP PRIMARY KEY,
        ADD PRIMARY KEY (owner_pid, companion_pid)',
    'DO 0');

PREPARE companion_migrate_stmt FROM @companion_migrate;
EXECUTE companion_migrate_stmt;
DEALLOCATE PREPARE companion_migrate_stmt;

-- Per-companion settings and the bond counters.
--
-- ADD COLUMN IF NOT EXISTS is a MariaDB extension and is a genuine no-op on a
-- table that already has the column, so this is safe on every start and on a
-- world created by any earlier version of this file.
--
--   stance   0 = aggressive (fights whatever the pack is fighting)
--            1 = defensive  (only what the owner attacked, or what hit us)
--            2 = passive    (never starts a fight)
--   profile  name of the team this companion belongs to; empty = always out
--   assists  monsters killed while this companion was engaged with them
--   seconds  time spent standing with its owner, in seconds
ALTER TABLE common.playerbot_companion
    ADD COLUMN IF NOT EXISTS stance  TINYINT UNSIGNED NOT NULL DEFAULT 0,
    ADD COLUMN IF NOT EXISTS profile VARCHAR(24)      NOT NULL DEFAULT '',
    ADD COLUMN IF NOT EXISTS assists INT UNSIGNED     NOT NULL DEFAULT 0,
    ADD COLUMN IF NOT EXISTS seconds INT UNSIGNED     NOT NULL DEFAULT 0;

-- Settings that belong to the owner rather than to one companion: the marching
-- formation, and which team profile is currently out. Kept in its own table so
-- an owner with no companions can still have settings, and so releasing the
-- last companion does not throw the owner's formation away with it.
--
--   formation 0 = wedge, 1 = line abreast, 2 = column, 3 = ring around the owner
CREATE TABLE IF NOT EXISTS common.playerbot_companion_owner (
    owner_pid INT UNSIGNED     NOT NULL,
    formation TINYINT UNSIGNED NOT NULL DEFAULT 0,
    profile   VARCHAR(24)      NOT NULL DEFAULT '',
    PRIMARY KEY (owner_pid)
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4;
