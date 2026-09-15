/*M!999999\- enable the sandbox mode */ 
-- MariaDB dump 10.19-11.8.2-MariaDB, for FreeBSD14.2 (amd64)
--
-- Host: localhost    Database: account
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
-- Table structure for table `GameTime`
--

DROP TABLE IF EXISTS `GameTime`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `GameTime` (
  `UserID` varchar(16) NOT NULL DEFAULT '',
  `paymenttype` tinyint(2) NOT NULL DEFAULT 1,
  `LimitTime` int(11) unsigned DEFAULT 0,
  `LimitDt` datetime DEFAULT current_timestamp(),
  `Scores` int(11) DEFAULT 0,
  PRIMARY KEY (`UserID`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `GameTime`
--

LOCK TABLES `GameTime` WRITE;
/*!40000 ALTER TABLE `GameTime` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `GameTime` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `GameTimeIP`
--

DROP TABLE IF EXISTS `GameTimeIP`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `GameTimeIP` (
  `ipid` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(128) DEFAULT NULL,
  `ip` varchar(15) NOT NULL DEFAULT '000.000.000.000',
  `startIP` int(11) NOT NULL DEFAULT 0,
  `endIP` int(11) NOT NULL DEFAULT 255,
  `paymenttype` tinyint(2) NOT NULL DEFAULT 1,
  `LimitTime` int(11) NOT NULL DEFAULT 0,
  `LimitDt` datetime NOT NULL DEFAULT current_timestamp(),
  `readme` varchar(128) DEFAULT NULL,
  PRIMARY KEY (`ipid`) USING BTREE,
  UNIQUE KEY `ip_uniq` (`ip`,`startIP`,`endIP`) USING BTREE,
  KEY `ip_idx` (`ip`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `GameTimeIP`
--

LOCK TABLES `GameTimeIP` WRITE;
/*!40000 ALTER TABLE `GameTimeIP` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `GameTimeIP` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `GameTimeLog`
--

DROP TABLE IF EXISTS `GameTimeLog`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `GameTimeLog` (
  `login` varchar(16) DEFAULT NULL,
  `type` enum('IP_FREE','FREE','IP_TIME','IP_DAY','TIME','DAY') DEFAULT NULL,
  `logon_time` datetime NOT NULL DEFAULT current_timestamp(),
  `logout_time` datetime NOT NULL DEFAULT current_timestamp(),
  `use_time` int(11) unsigned DEFAULT NULL,
  `ip` varchar(15) NOT NULL DEFAULT '000.000.000.000',
  `server` varchar(56) NOT NULL DEFAULT '',
  KEY `login_key` (`login`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `GameTimeLog`
--

LOCK TABLES `GameTimeLog` WRITE;
/*!40000 ALTER TABLE `GameTimeLog` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `GameTimeLog` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `account`
--

DROP TABLE IF EXISTS `account`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `account` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `login` varchar(16) NOT NULL DEFAULT '' COMMENT 'LOGIN_MAX_LEN=30',
  `password` varchar(42) NOT NULL DEFAULT '' COMMENT 'PASSWD_MAX_LEN=16; default 45 size',
  `social_id` varchar(7) NOT NULL DEFAULT '',
  `email` varchar(100) NOT NULL DEFAULT '',
  `securitycode` varchar(192) NOT NULL DEFAULT '',
  `status` varchar(8) NOT NULL DEFAULT 'OK',
  `availDt` datetime NOT NULL DEFAULT current_timestamp(),
  `create_time` datetime NOT NULL DEFAULT current_timestamp(),
  `last_play` datetime NOT NULL DEFAULT current_timestamp(),
  `premium_expire` datetime NOT NULL DEFAULT current_timestamp(),
  `gold_expire` datetime NOT NULL DEFAULT current_timestamp(),
  `silver_expire` datetime NOT NULL DEFAULT current_timestamp(),
  `safebox_expire` datetime NOT NULL DEFAULT current_timestamp(),
  `autoloot_expire` datetime NOT NULL DEFAULT current_timestamp(),
  `fish_mind_expire` datetime NOT NULL DEFAULT current_timestamp(),
  `marriage_fast_expire` datetime NOT NULL DEFAULT current_timestamp(),
  `money_drop_rate_expire` datetime NOT NULL DEFAULT current_timestamp(),
  `shop_expire` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  `real_name` varchar(16) DEFAULT '',
  `question1` varchar(56) DEFAULT NULL,
  `answer1` varchar(56) DEFAULT NULL,
  `question2` varchar(56) DEFAULT NULL,
  `answer2` varchar(56) DEFAULT NULL,
  `cash` int(10) DEFAULT 0,
  `cash_mark` int(10) DEFAULT 0,
  `web_user_id` varchar(255) DEFAULT NULL,
  `hwid` varchar(255) DEFAULT '',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE KEY `login` (`login`) USING BTREE,
  KEY `social_id` (`social_id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `account`
--

LOCK TABLES `account` WRITE;
/*!40000 ALTER TABLE `account` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `account` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `account_block`
--

DROP TABLE IF EXISTS `account_block`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `account_block` (
  `account_id` int(11) NOT NULL,
  `ban_duration` int(11) NOT NULL DEFAULT 0,
  `pid` int(11) NOT NULL DEFAULT 0,
  `who` int(11) NOT NULL DEFAULT 0,
  `reason` varchar(255) NOT NULL DEFAULT '',
  `status` tinyint(1) NOT NULL DEFAULT 0,
  `flush_time` datetime DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=cp1250 COLLATE=cp1250_polish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `account_block`
--

LOCK TABLES `account_block` WRITE;
/*!40000 ALTER TABLE `account_block` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `account_block` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `block_exception`
--

DROP TABLE IF EXISTS `block_exception`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `block_exception` (
  `login` varchar(16) NOT NULL DEFAULT ''
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `block_exception`
--

LOCK TABLES `block_exception` WRITE;
/*!40000 ALTER TABLE `block_exception` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `block_exception` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `hardware_status`
--

DROP TABLE IF EXISTS `hardware_status`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `hardware_status` (
  `hwid` varchar(255) NOT NULL,
  `status` varchar(8) NOT NULL DEFAULT 'OK',
  PRIMARY KEY (`hwid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `hardware_status`
--

LOCK TABLES `hardware_status` WRITE;
/*!40000 ALTER TABLE `hardware_status` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `hardware_status` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `iptocountry`
--

DROP TABLE IF EXISTS `iptocountry`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `iptocountry` (
  `IP_FROM` varchar(16) DEFAULT NULL,
  `IP_TO` varchar(16) DEFAULT NULL,
  `COUNTRY_NAME` varchar(56) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `iptocountry`
--

LOCK TABLES `iptocountry` WRITE;
/*!40000 ALTER TABLE `iptocountry` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `iptocountry` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `string`
--

DROP TABLE IF EXISTS `string`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `string` (
  `name` varchar(64) NOT NULL DEFAULT '',
  `text` text DEFAULT NULL,
  PRIMARY KEY (`name`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `string`
--

LOCK TABLES `string` WRITE;
/*!40000 ALTER TABLE `string` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `string` ENABLE KEYS */;
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

-- Dump completed on 2026-02-07 15:11:05
