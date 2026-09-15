/*M!999999\- enable the sandbox mode */ 
-- MariaDB dump 10.19-11.8.2-MariaDB, for FreeBSD14.2 (amd64)
--
-- Host: localhost    Database: log
-- ------------------------------------------------------
-- Server version	11.8.2-MariaDB

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*M!100616 SET @OLD_NOTE_VERBOSITY=@@NOTE_VERBOSITY, NOTE_VERBOSITY=0 */;

--
-- Table structure for table `bright_protect_log`
--

DROP TABLE IF EXISTS `bright_protect_log`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `bright_protect_log` (
  `date` datetime DEFAULT NULL,
  `login` varchar(32) DEFAULT NULL,
  `name` varchar(32) DEFAULT NULL,
  `IP` text CHARACTER SET latin1 COLLATE latin1_swedish_ci DEFAULT NULL,
  `cheat` varchar(22) DEFAULT NULL,
  `value` int(9) NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=big5 COLLATE=big5_bin;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `bright_protect_log`
--

LOCK TABLES `bright_protect_log` WRITE;
/*!40000 ALTER TABLE `bright_protect_log` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `bright_protect_log` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `change_name`
--

DROP TABLE IF EXISTS `change_name`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `change_name` (
  `pid` int(20) NOT NULL DEFAULT 0,
  `old_name` varchar(30) DEFAULT NULL,
  `new_name` varchar(30) DEFAULT NULL,
  `time` datetime DEFAULT NULL,
  `ip` varchar(30) DEFAULT NULL,
  PRIMARY KEY (`pid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin2 COLLATE=latin2_general_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `change_name`
--

LOCK TABLES `change_name` WRITE;
/*!40000 ALTER TABLE `change_name` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `change_name` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `hack_log`
--

DROP TABLE IF EXISTS `hack_log`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `hack_log` (
  `time` datetime DEFAULT NULL,
  `name` varbinary(16) DEFAULT NULL,
  `server` varbinary(56) DEFAULT NULL,
  `why` varbinary(300) DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `hack_log`
--

LOCK TABLES `hack_log` WRITE;
/*!40000 ALTER TABLE `hack_log` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `hack_log` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `itemshop_log`
--

DROP TABLE IF EXISTS `itemshop_log`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `itemshop_log` (
  `account_id` int(11) DEFAULT NULL,
  `vnum` int(11) DEFAULT NULL,
  `count` int(11) DEFAULT NULL,
  `price` int(11) DEFAULT NULL,
  `id` int(11) DEFAULT NULL,
  `category` int(11) DEFAULT NULL,
  `channel` smallint(6) DEFAULT NULL,
  `date` datetime DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=COMPACT;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `itemshop_log`
--

LOCK TABLES `itemshop_log` WRITE;
/*!40000 ALTER TABLE `itemshop_log` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `itemshop_log` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `log`
--

DROP TABLE IF EXISTS `log`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `log` (
  `type` varbinary(20) DEFAULT NULL COMMENT 'contains: CHARACTER, ITEM',
  `time` datetime DEFAULT NULL,
  `who` int(11) unsigned DEFAULT NULL,
  `x` int(11) unsigned DEFAULT NULL,
  `y` int(11) unsigned DEFAULT NULL,
  `what` bigint(11) unsigned DEFAULT NULL,
  `how` varbinary(50) DEFAULT NULL,
  `hint` varbinary(500) DEFAULT NULL,
  `vnum` int(11) unsigned DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `log`
--

LOCK TABLES `log` WRITE;
/*!40000 ALTER TABLE `log` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `log` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `log_chat`
--

DROP TABLE IF EXISTS `log_chat`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `log_chat` (
  `where` int(10) unsigned NOT NULL,
  `who_id` int(10) unsigned NOT NULL,
  `type` enum('NORMAL','WHISPER','PARTY','GUILD','SHOUT','OFFLINE_WHISPER') NOT NULL,
  `msg` varbinary(512) NOT NULL,
  `when` datetime NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `log_chat`
--

LOCK TABLES `log_chat` WRITE;
/*!40000 ALTER TABLE `log_chat` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `log_chat` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `log_chat_priv`
--

DROP TABLE IF EXISTS `log_chat_priv`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `log_chat_priv` (
  `where` int(10) unsigned NOT NULL,
  `who_id` int(10) unsigned NOT NULL,
  `who_name` varchar(25) CHARACTER SET utf8mb3 COLLATE utf8mb3_uca1400_ai_ci NOT NULL,
  `whom_id` int(10) unsigned DEFAULT NULL,
  `whom_name` varchar(25) CHARACTER SET utf8mb3 COLLATE utf8mb3_uca1400_ai_ci NOT NULL,
  `type` enum('NORMAL','WHISPER','PARTY','GUILD','SHOUT','OFFLINE_WHISPER') NOT NULL,
  `msg` varbinary(512) NOT NULL,
  `when` datetime NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `log_chat_priv`
--

LOCK TABLES `log_chat_priv` WRITE;
/*!40000 ALTER TABLE `log_chat_priv` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `log_chat_priv` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `log_commands`
--

DROP TABLE IF EXISTS `log_commands`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `log_commands` (
  `userid` int(11) DEFAULT NULL,
  `port` int(11) DEFAULT NULL,
  `command` varbinary(1024) DEFAULT NULL,
  `date` datetime DEFAULT '0000-00-00 00:00:00'
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `log_commands`
--

LOCK TABLES `log_commands` WRITE;
/*!40000 ALTER TABLE `log_commands` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `log_commands` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `log_dungeon`
--

DROP TABLE IF EXISTS `log_dungeon`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `log_dungeon` (
  `name` varbinary(20) DEFAULT NULL COMMENT 'contains: CHARACTER, ITEM',
  `time` datetime DEFAULT NULL,
  `mob` bigint(11) unsigned DEFAULT NULL,
  `map` varbinary(80) DEFAULT NULL COMMENT 'snprintf with 80u size',
  `x` int(11) unsigned DEFAULT NULL,
  `y` int(11) unsigned DEFAULT NULL,
  `how` varbinary(50) DEFAULT NULL,
  `id` int(11) unsigned DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `log_dungeon`
--

LOCK TABLES `log_dungeon` WRITE;
/*!40000 ALTER TABLE `log_dungeon` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `log_dungeon` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `log_level`
--

DROP TABLE IF EXISTS `log_level`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `log_level` (
  `name` varchar(50) DEFAULT NULL,
  `level` int(11) DEFAULT NULL,
  `time` time DEFAULT NULL,
  `playtime` int(11) DEFAULT NULL,
  `account_id` int(11) DEFAULT NULL,
  `pid` int(11) DEFAULT NULL,
  `id` int(11) NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=big5 COLLATE=big5_bin;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `log_level`
--

LOCK TABLES `log_level` WRITE;
/*!40000 ALTER TABLE `log_level` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `log_level` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `log_quest`
--

DROP TABLE IF EXISTS `log_quest`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `log_quest` (
  `1` varchar(50) DEFAULT NULL,
  `2` int(11) DEFAULT NULL,
  `3` int(11) DEFAULT NULL,
  `4` int(11) DEFAULT NULL,
  `5` int(11) DEFAULT NULL,
  `6` int(11) DEFAULT NULL,
  `7` time DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=big5 COLLATE=big5_bin;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `log_quest`
--

LOCK TABLES `log_quest` WRITE;
/*!40000 ALTER TABLE `log_quest` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `log_quest` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `new_money_log`
--

DROP TABLE IF EXISTS `new_money_log`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `new_money_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `char_id` int(11) DEFAULT NULL,
  `player_name` varchar(255) DEFAULT NULL,
  `type` varchar(255) DEFAULT NULL,
  `time` datetime DEFAULT NULL,
  `add_info` varchar(255) DEFAULT NULL,
  `vnum` int(11) DEFAULT NULL,
  `with_id` int(11) DEFAULT NULL,
  `with_name` varchar(255) DEFAULT NULL,
  `transaction_gold` varchar(255) CHARACTER SET latin1 COLLATE latin1_swedish_ci DEFAULT NULL,
  `pgold_before_transaction` varchar(255) CHARACTER SET latin1 COLLATE latin1_swedish_ci DEFAULT NULL,
  `pgold_after_transaction` varchar(255) CHARACTER SET latin1 COLLATE latin1_swedish_ci DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb3 COLLATE=utf8mb3_uca1400_ai_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `new_money_log`
--

LOCK TABLES `new_money_log` WRITE;
/*!40000 ALTER TABLE `new_money_log` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `new_money_log` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `offline_shop`
--

DROP TABLE IF EXISTS `offline_shop`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `offline_shop` (
  `owner_pid` int(11) DEFAULT NULL,
  `shop_id` int(11) DEFAULT NULL,
  `type` varchar(50) DEFAULT NULL,
  `buyer_pid` int(11) DEFAULT NULL,
  `item_id` int(11) DEFAULT NULL,
  `vnum` int(11) DEFAULT NULL,
  `count` int(11) DEFAULT NULL,
  `gold` bigint(20) DEFAULT NULL,
  `cheque` bigint(20) DEFAULT NULL,
  `time` datetime DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `offline_shop`
--

LOCK TABLES `offline_shop` WRITE;
/*!40000 ALTER TABLE `offline_shop` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `offline_shop` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `okay_event`
--

DROP TABLE IF EXISTS `okay_event`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `okay_event` (
  `pid` int(11) DEFAULT NULL,
  `name` varchar(11) DEFAULT NULL,
  `points` int(11) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `okay_event`
--

LOCK TABLES `okay_event` WRITE;
/*!40000 ALTER TABLE `okay_event` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `okay_event` ENABLE KEYS */;
UNLOCK TABLES;
commit;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*M!100616 SET NOTE_VERBOSITY=@OLD_NOTE_VERBOSITY */;

-- Dump completed on 2026-02-07 15:11:06
