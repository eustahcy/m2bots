/*M!999999\- enable the sandbox mode */ 
-- MariaDB dump 10.19-11.8.2-MariaDB, for FreeBSD14.2 (amd64)
--
-- Host: localhost    Database: player
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
-- Table structure for table `affect`
--

DROP TABLE IF EXISTS `affect`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `affect` (
  `dwPID` int(10) unsigned NOT NULL DEFAULT 0,
  `bType` smallint(5) unsigned NOT NULL DEFAULT 0,
  `bApplyOn` tinyint(4) unsigned NOT NULL DEFAULT 0,
  `lApplyValue` int(11) NOT NULL DEFAULT 0,
  `dwFlag` int(10) unsigned NOT NULL DEFAULT 0,
  `lDuration` int(10) NOT NULL DEFAULT 0,
  `lSPCost` int(10) NOT NULL DEFAULT 0,
  `itemVnum` int(10) unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`dwPID`,`bType`,`bApplyOn`,`lApplyValue`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `affect`
--

LOCK TABLES `affect` WRITE;
/*!40000 ALTER TABLE `affect` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `affect` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `change_empire`
--

DROP TABLE IF EXISTS `change_empire`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `change_empire` (
  `aid` int(10) unsigned NOT NULL,
  `change_count` int(10) unsigned NOT NULL DEFAULT 0,
  `last_time` datetime NOT NULL DEFAULT '0000-00-00 00:00:00' ON UPDATE current_timestamp(),
  PRIMARY KEY (`aid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `change_empire`
--

LOCK TABLES `change_empire` WRITE;
/*!40000 ALTER TABLE `change_empire` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `change_empire` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `guild`
--

DROP TABLE IF EXISTS `guild`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `guild` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(14) NOT NULL DEFAULT '' COMMENT 'snprintf(12u)',
  `sp` smallint(6) NOT NULL DEFAULT 1000,
  `master` int(10) unsigned NOT NULL DEFAULT 0,
  `level` tinyint(2) DEFAULT NULL,
  `exp` int(11) DEFAULT NULL,
  `skill_point` tinyint(2) NOT NULL DEFAULT 0,
  `skill` tinyblob DEFAULT NULL,
  `win` int(11) NOT NULL DEFAULT 0,
  `draw` int(11) NOT NULL DEFAULT 0,
  `loss` int(11) NOT NULL DEFAULT 0,
  `ladder_point` int(11) NOT NULL DEFAULT 0,
  `gold` bigint(19) NOT NULL DEFAULT 0,
  `token` bigint(20) unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `guild`
--

LOCK TABLES `guild` WRITE;
/*!40000 ALTER TABLE `guild` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `guild` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `guild_comment`
--

DROP TABLE IF EXISTS `guild_comment`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `guild_comment` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `guild_id` int(10) unsigned DEFAULT NULL,
  `name` varchar(24) DEFAULT NULL,
  `notice` tinyint(4) DEFAULT NULL,
  `content` varchar(50) DEFAULT NULL,
  `time` datetime DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  KEY `aaa` (`notice`,`id`,`guild_id`) USING BTREE,
  KEY `guild_id` (`guild_id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=cp1250 COLLATE=cp1250_polish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `guild_comment`
--

LOCK TABLES `guild_comment` WRITE;
/*!40000 ALTER TABLE `guild_comment` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `guild_comment` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `guild_grade`
--

DROP TABLE IF EXISTS `guild_grade`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `guild_grade` (
  `guild_id` int(11) unsigned NOT NULL DEFAULT 0,
  `grade` tinyint(4) NOT NULL DEFAULT 0,
  `name` varchar(12) NOT NULL DEFAULT '' COMMENT 'strlen(s) <= 12',
  `auth` set('ADD_MEMBER','REMOVE_MEMEBER','NOTICE','USE_SKILL') CHARACTER SET latin1 COLLATE latin1_swedish_ci DEFAULT NULL,
  PRIMARY KEY (`guild_id`,`grade`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=cp1250 COLLATE=cp1250_polish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `guild_grade`
--

LOCK TABLES `guild_grade` WRITE;
/*!40000 ALTER TABLE `guild_grade` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `guild_grade` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `guild_land`
--

DROP TABLE IF EXISTS `guild_land`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `guild_land` (
  `land_id` int(11) NOT NULL DEFAULT 0,
  `guild_id` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`land_id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `guild_land`
--

LOCK TABLES `guild_land` WRITE;
/*!40000 ALTER TABLE `guild_land` DISABLE KEYS */;
set autocommit=0;
INSERT INTO `guild_land` VALUES
(2,408),
(8,78),
(9,108),
(10,69),
(14,3),
(15,395),
(16,2),
(17,52),
(18,18),
(108,5),
(109,6),
(115,92),
(116,93),
(117,20),
(118,13),
(201,212),
(204,712),
(205,57),
(206,9),
(207,58),
(208,25),
(212,19),
(213,14),
(214,15),
(215,344),
(216,47),
(217,33),
(218,7);
/*!40000 ALTER TABLE `guild_land` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `guild_member`
--

DROP TABLE IF EXISTS `guild_member`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `guild_member` (
  `pid` int(10) unsigned NOT NULL DEFAULT 0,
  `guild_id` int(10) unsigned NOT NULL DEFAULT 0,
  `grade` tinyint(2) DEFAULT NULL,
  `is_general` tinyint(1) NOT NULL DEFAULT 0,
  `offer` int(10) unsigned DEFAULT NULL,
  PRIMARY KEY (`guild_id`,`pid`) USING BTREE,
  UNIQUE KEY `pid` (`pid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `guild_member`
--

LOCK TABLES `guild_member` WRITE;
/*!40000 ALTER TABLE `guild_member` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `guild_member` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `guild_war`
--

DROP TABLE IF EXISTS `guild_war`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `guild_war` (
  `id_from` int(11) unsigned NOT NULL DEFAULT 0,
  `id_to` int(11) unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`id_from`,`id_to`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `guild_war`
--

LOCK TABLES `guild_war` WRITE;
/*!40000 ALTER TABLE `guild_war` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `guild_war` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `guild_war_bet`
--

DROP TABLE IF EXISTS `guild_war_bet`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `guild_war_bet` (
  `login` varchar(16) NOT NULL DEFAULT '',
  `gold` bigint(19) unsigned NOT NULL DEFAULT 0,
  `guild` int(11) unsigned NOT NULL DEFAULT 0,
  `war_id` int(10) unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`war_id`,`login`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `guild_war_bet`
--

LOCK TABLES `guild_war_bet` WRITE;
/*!40000 ALTER TABLE `guild_war_bet` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `guild_war_bet` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `guild_war_reservation`
--

DROP TABLE IF EXISTS `guild_war_reservation`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `guild_war_reservation` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `guild1` int(10) unsigned NOT NULL DEFAULT 0,
  `guild2` int(10) unsigned NOT NULL DEFAULT 0,
  `time` datetime NOT NULL DEFAULT current_timestamp(),
  `type` tinyint(2) unsigned NOT NULL DEFAULT 0,
  `warprice` int(10) unsigned NOT NULL DEFAULT 0,
  `initscore` int(10) unsigned NOT NULL DEFAULT 0,
  `started` tinyint(1) NOT NULL DEFAULT 0,
  `bet_from` int(10) unsigned NOT NULL DEFAULT 0,
  `bet_to` int(10) unsigned NOT NULL DEFAULT 0,
  `winner` int(11) NOT NULL DEFAULT -1,
  `power1` int(11) NOT NULL DEFAULT 0,
  `power2` int(11) NOT NULL DEFAULT 0,
  `handicap` int(11) NOT NULL DEFAULT 0,
  `result1` int(11) NOT NULL DEFAULT 0,
  `result2` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=36 DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `guild_war_reservation`
--

LOCK TABLES `guild_war_reservation` WRITE;
/*!40000 ALTER TABLE `guild_war_reservation` DISABLE KEYS */;
set autocommit=0;
INSERT INTO `guild_war_reservation` VALUES
(1,34,95,'2025-02-08 02:06:50',1,0,0,1,0,0,95,0,0,0,404,3355),
(2,2,7,'2025-02-09 01:43:05',1,0,0,1,0,0,7,0,0,0,295,2177),
(3,43,95,'2025-02-09 21:09:12',1,0,0,1,0,0,95,0,0,0,299,3663),
(4,7,95,'2025-02-10 21:19:53',1,0,0,1,0,0,7,0,0,0,4666,897),
(5,64,95,'2025-02-10 22:13:47',1,0,0,1,0,0,95,0,0,0,162,682),
(6,30,95,'2025-02-11 17:04:15',1,0,0,1,0,0,95,0,0,0,278,1363),
(7,52,95,'2025-02-11 20:43:20',1,0,0,1,0,0,52,0,0,0,4342,3467),
(8,92,95,'2025-02-13 21:12:40',1,0,0,1,0,0,0,0,0,0,0,0),
(9,92,95,'2025-02-13 21:16:06',1,0,0,1,0,0,95,0,0,0,0,0),
(10,92,95,'2025-02-13 21:32:03',1,0,0,1,0,0,0,0,0,0,0,0),
(11,9,95,'2025-02-16 00:18:30',1,0,0,1,0,0,95,0,0,0,888,5072),
(12,7,64,'2025-02-16 18:40:16',1,0,0,1,0,0,0,0,0,0,0,0),
(13,47,95,'2025-02-16 20:56:42',1,0,0,1,0,0,0,0,0,0,0,0),
(14,78,95,'2025-02-17 19:15:48',1,0,0,1,0,0,0,0,0,0,0,0),
(15,78,95,'2025-02-17 19:23:05',1,0,0,1,0,0,0,0,0,0,0,0),
(16,47,78,'2025-02-17 21:36:14',1,0,0,1,0,0,47,0,0,0,4964,730),
(17,39,47,'2025-02-17 22:34:29',1,0,0,1,0,0,47,0,0,0,1258,5570),
(18,63,331,'2025-02-21 22:55:50',1,0,0,1,0,0,0,0,0,0,0,0),
(19,47,331,'2025-02-21 23:02:14',1,0,0,1,0,0,0,0,0,0,0,0),
(20,131,370,'2025-02-25 02:09:36',1,0,0,1,0,0,370,0,0,0,1604,2498),
(21,64,78,'2025-02-25 21:11:07',1,0,0,1,0,0,0,0,0,0,0,0),
(22,78,538,'2025-02-25 21:46:15',1,0,0,1,0,0,0,0,0,0,0,0),
(23,47,78,'2025-02-25 21:52:09',1,0,0,1,0,0,0,0,0,0,0,0),
(24,78,95,'2025-03-04 18:55:18',1,0,0,1,0,0,0,0,0,0,0,0),
(25,47,78,'2025-03-06 22:48:27',1,0,0,1,0,0,47,0,0,0,5800,411),
(26,78,95,'2025-03-07 23:20:28',1,0,0,1,0,0,0,0,0,0,0,0),
(27,9,57,'2025-03-07 23:20:54',1,0,0,1,0,0,0,0,0,0,0,0),
(28,78,95,'2025-03-07 23:30:14',1,0,0,1,0,0,78,0,0,0,6034,3147),
(29,78,92,'2025-03-08 20:38:18',1,0,0,1,0,0,92,0,0,0,4382,5287),
(30,3,78,'2025-03-08 21:15:23',1,0,0,1,0,0,3,0,0,0,5988,503),
(31,7,47,'2025-03-10 20:37:12',1,0,0,1,0,0,0,0,0,0,0,0),
(32,7,47,'2025-03-10 21:06:06',1,0,0,1,0,0,7,0,0,0,6864,3439),
(33,131,370,'2025-03-13 22:28:38',1,0,0,1,0,0,131,0,0,0,5753,4232),
(34,9,78,'2025-03-13 22:30:37',1,0,0,1,0,0,9,0,0,0,6549,2854),
(35,23,92,'2025-03-16 17:09:37',1,0,0,1,0,0,23,0,0,0,6018,1180);
/*!40000 ALTER TABLE `guild_war_reservation` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `horse_name`
--

DROP TABLE IF EXISTS `horse_name`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `horse_name` (
  `id` int(11) unsigned NOT NULL DEFAULT 0,
  `name` varchar(16) NOT NULL DEFAULT 'NONAME' COMMENT 'CHARACTER_NAME_MAX_LEN+1 so 24+null',
  PRIMARY KEY (`id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `horse_name`
--

LOCK TABLES `horse_name` WRITE;
/*!40000 ALTER TABLE `horse_name` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `horse_name` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `ikashop_auction_offer`
--

DROP TABLE IF EXISTS `ikashop_auction_offer`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `ikashop_auction_offer` (
  `seller` int(10) unsigned NOT NULL,
  `buyer` int(10) unsigned NOT NULL,
  `buyer_name` varchar(30) NOT NULL,
  `price` bigint(20) unsigned NOT NULL,
  `datetime` datetime NOT NULL DEFAULT current_timestamp() ON UPDATE current_timestamp(),
  `seen` tinyint(3) unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`seller`,`buyer`,`price`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `ikashop_auction_offer`
--

LOCK TABLES `ikashop_auction_offer` WRITE;
/*!40000 ALTER TABLE `ikashop_auction_offer` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `ikashop_auction_offer` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `ikashop_notification`
--

DROP TABLE IF EXISTS `ikashop_notification`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `ikashop_notification` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `owner` int(10) unsigned NOT NULL,
  `type` int(10) unsigned NOT NULL,
  `who` varchar(32) NOT NULL,
  `what` int(10) unsigned NOT NULL,
  `format` varchar(50) NOT NULL,
  `seen` tinyint(4) NOT NULL DEFAULT 0,
  `datetime` datetime NOT NULL DEFAULT current_timestamp(),
  PRIMARY KEY (`id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `ikashop_notification`
--

LOCK TABLES `ikashop_notification` WRITE;
/*!40000 ALTER TABLE `ikashop_notification` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `ikashop_notification` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `ikashop_offlineshop`
--

DROP TABLE IF EXISTS `ikashop_offlineshop`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `ikashop_offlineshop` (
  `owner` int(10) unsigned NOT NULL,
  `duration` int(10) unsigned NOT NULL DEFAULT 0,
  `map` int(10) unsigned NOT NULL DEFAULT 0,
  `x` int(10) unsigned NOT NULL DEFAULT 0,
  `y` int(10) unsigned NOT NULL DEFAULT 0,
  `channel` tinyint(3) NOT NULL DEFAULT 1,
  `decoration` int(10) unsigned NOT NULL DEFAULT 0,
  `decoration_time` int(10) unsigned NOT NULL DEFAULT 0,
  `lock_index` int(10) unsigned NOT NULL DEFAULT 0,
  `name` varchar(32) NOT NULL DEFAULT 'NONAME',
  `unlock_count` tinyint(3) NOT NULL DEFAULT 0,
  `is_premium` tinyint(1) NOT NULL DEFAULT 0,
  PRIMARY KEY (`owner`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=cp1250 COLLATE=cp1250_polish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `ikashop_offlineshop`
--

LOCK TABLES `ikashop_offlineshop` WRITE;
/*!40000 ALTER TABLE `ikashop_offlineshop` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `ikashop_offlineshop` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `ikashop_private_offer`
--

DROP TABLE IF EXISTS `ikashop_private_offer`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `ikashop_private_offer` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `seller` int(10) unsigned NOT NULL,
  `buyer` int(10) unsigned NOT NULL,
  `item` int(10) unsigned NOT NULL,
  `price` bigint(20) unsigned NOT NULL,
  `notified` tinyint(3) unsigned NOT NULL,
  `accepted` tinyint(3) unsigned NOT NULL,
  `buyer_name` varchar(30) NOT NULL,
  `datetime` datetime DEFAULT current_timestamp(),
  PRIMARY KEY (`id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `ikashop_private_offer`
--

LOCK TABLES `ikashop_private_offer` WRITE;
/*!40000 ALTER TABLE `ikashop_private_offer` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `ikashop_private_offer` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `ikashop_safebox`
--

DROP TABLE IF EXISTS `ikashop_safebox`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `ikashop_safebox` (
  `owner` int(10) unsigned NOT NULL,
  `gold` bigint(20) unsigned NOT NULL,
  `cheque` int(10) unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`owner`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `ikashop_safebox`
--

LOCK TABLES `ikashop_safebox` WRITE;
/*!40000 ALTER TABLE `ikashop_safebox` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `ikashop_safebox` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `ikashop_sale`
--

DROP TABLE IF EXISTS `ikashop_sale`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `ikashop_sale` (
  `id` int(10) unsigned NOT NULL,
  `vnum` int(10) unsigned NOT NULL,
  `count` int(10) unsigned NOT NULL,
  `price` bigint(20) unsigned NOT NULL,
  `datetime` datetime NOT NULL DEFAULT current_timestamp()
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `ikashop_sale`
--

LOCK TABLES `ikashop_sale` WRITE;
/*!40000 ALTER TABLE `ikashop_sale` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `ikashop_sale` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `item`
--

DROP TABLE IF EXISTS `item`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `item` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `owner_id` int(11) unsigned NOT NULL DEFAULT 0,
  `window` enum('INVENTORY','EQUIPMENT','SAFEBOX','MALL','DRAGON_SOUL_INVENTORY','BELT_INVENTORY','GROUND','IKASHOP_OFFLINESHOP','IKASHOP_SAFEBOX','IKASHOP_AUCTION') NOT NULL,
  `pos` smallint(5) unsigned NOT NULL DEFAULT 0,
  `count` int(10) unsigned NOT NULL DEFAULT 0,
  `vnum` int(11) unsigned NOT NULL DEFAULT 0,
  `socket0` int(10) unsigned NOT NULL DEFAULT 0,
  `socket1` int(10) unsigned NOT NULL DEFAULT 0,
  `socket2` int(10) unsigned NOT NULL DEFAULT 0,
  `attrtype0` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `attrvalue0` smallint(6) NOT NULL DEFAULT 0,
  `attrtype1` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `attrvalue1` smallint(6) NOT NULL DEFAULT 0,
  `attrtype2` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `attrvalue2` smallint(6) NOT NULL DEFAULT 0,
  `attrtype3` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `attrvalue3` smallint(6) NOT NULL DEFAULT 0,
  `attrtype4` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `attrvalue4` smallint(6) NOT NULL DEFAULT 0,
  `attrtype5` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `attrvalue5` smallint(6) NOT NULL DEFAULT 0,
  `attrtype6` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `attrvalue6` smallint(6) NOT NULL DEFAULT 0,
  `ikashop_data` tinytext DEFAULT '',
  PRIMARY KEY (`id`) USING BTREE,
  KEY `owner_id_idx` (`owner_id`) USING BTREE,
  KEY `item_vnum_index` (`vnum`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `item`
--

LOCK TABLES `item` WRITE;
/*!40000 ALTER TABLE `item` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `item` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `item_award`
--

DROP TABLE IF EXISTS `item_award`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `item_award` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `login` varchar(16) NOT NULL DEFAULT '' COMMENT 'LOGIN_MAX_LEN=30',
  `vnum` int(6) unsigned NOT NULL DEFAULT 0,
  `count` int(10) unsigned NOT NULL DEFAULT 1,
  `given_time` datetime NOT NULL DEFAULT current_timestamp(),
  `taken_time` datetime DEFAULT NULL,
  `item_id` int(11) unsigned DEFAULT NULL,
  `why` varchar(128) DEFAULT NULL,
  `socket0` int(11) unsigned NOT NULL DEFAULT 0,
  `socket1` int(11) unsigned NOT NULL DEFAULT 0,
  `socket2` int(11) unsigned NOT NULL DEFAULT 0,
  `attrtype0` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `attrvalue0` smallint(6) NOT NULL DEFAULT 0,
  `attrtype1` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `attrvalue1` smallint(6) NOT NULL DEFAULT 0,
  `attrtype2` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `attrvalue2` smallint(6) NOT NULL DEFAULT 0,
  `attrtype3` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `attrvalue3` smallint(6) NOT NULL DEFAULT 0,
  `attrtype4` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `attrvalue4` smallint(6) NOT NULL DEFAULT 0,
  `attrtype5` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `attrvalue5` smallint(6) NOT NULL DEFAULT 0,
  `attrtype6` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `attrvalue6` smallint(6) NOT NULL DEFAULT 0,
  `mall` tinyint(1) NOT NULL DEFAULT 1,
  PRIMARY KEY (`id`) USING BTREE,
  KEY `given_time_idx` (`given_time`) USING BTREE,
  KEY `taken_time_idx` (`taken_time`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `item_award`
--

LOCK TABLES `item_award` WRITE;
/*!40000 ALTER TABLE `item_award` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `item_award` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `itemshop_time_auction`
--

DROP TABLE IF EXISTS `itemshop_time_auction`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `itemshop_time_auction` (
  `item_index` int(10) NOT NULL,
  `buy_count` int(10) NOT NULL DEFAULT 0,
  PRIMARY KEY (`item_index`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_general_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `itemshop_time_auction`
--

LOCK TABLES `itemshop_time_auction` WRITE;
/*!40000 ALTER TABLE `itemshop_time_auction` DISABLE KEYS */;
set autocommit=0;
INSERT INTO `itemshop_time_auction` VALUES
(906,0),
(907,0),
(908,0);
/*!40000 ALTER TABLE `itemshop_time_auction` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `lotto_list`
--

DROP TABLE IF EXISTS `lotto_list`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `lotto_list` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `server` varchar(56) DEFAULT NULL COMMENT 'server%s=get_table_postfix(); std::string so dunno; at least 6',
  `pid` int(10) unsigned DEFAULT NULL,
  `time` datetime DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `lotto_list`
--

LOCK TABLES `lotto_list` WRITE;
/*!40000 ALTER TABLE `lotto_list` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `lotto_list` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `marriage`
--

DROP TABLE IF EXISTS `marriage`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `marriage` (
  `is_married` tinyint(4) NOT NULL DEFAULT 0,
  `pid1` int(10) unsigned NOT NULL DEFAULT 0,
  `pid2` int(10) unsigned NOT NULL DEFAULT 0,
  `love_point` int(11) unsigned DEFAULT NULL,
  `time` int(10) unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`pid1`,`pid2`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `marriage`
--

LOCK TABLES `marriage` WRITE;
/*!40000 ALTER TABLE `marriage` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `marriage` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `messenger_block_list`
--

DROP TABLE IF EXISTS `messenger_block_list`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `messenger_block_list` (
  `pid` int(11) NOT NULL,
  `blocked` varchar(24) NOT NULL,
  PRIMARY KEY (`pid`,`blocked`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=cp1250 COLLATE=cp1250_general_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `messenger_block_list`
--

LOCK TABLES `messenger_block_list` WRITE;
/*!40000 ALTER TABLE `messenger_block_list` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `messenger_block_list` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `messenger_list`
--

DROP TABLE IF EXISTS `messenger_list`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `messenger_list` (
  `account` varchar(16) NOT NULL DEFAULT '' COMMENT '24 at maximum',
  `companion` varchar(16) NOT NULL DEFAULT '' COMMENT '24 at maximum',
  PRIMARY KEY (`account`,`companion`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `messenger_list`
--

LOCK TABLES `messenger_list` WRITE;
/*!40000 ALTER TABLE `messenger_list` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `messenger_list` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `monarch`
--

DROP TABLE IF EXISTS `monarch`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `monarch` (
  `empire` int(10) unsigned NOT NULL DEFAULT 0,
  `pid` int(10) unsigned DEFAULT NULL,
  `windate` datetime DEFAULT NULL,
  `money` bigint(20) unsigned DEFAULT NULL,
  PRIMARY KEY (`empire`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `monarch`
--

LOCK TABLES `monarch` WRITE;
/*!40000 ALTER TABLE `monarch` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `monarch` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `monarch_candidacy`
--

DROP TABLE IF EXISTS `monarch_candidacy`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `monarch_candidacy` (
  `pid` int(10) unsigned NOT NULL DEFAULT 0,
  `date` datetime DEFAULT current_timestamp(),
  `name` varchar(16) DEFAULT NULL,
  `windate` datetime DEFAULT NULL,
  PRIMARY KEY (`pid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `monarch_candidacy`
--

LOCK TABLES `monarch_candidacy` WRITE;
/*!40000 ALTER TABLE `monarch_candidacy` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `monarch_candidacy` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `monarch_election`
--

DROP TABLE IF EXISTS `monarch_election`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `monarch_election` (
  `pid` int(10) unsigned NOT NULL DEFAULT 0,
  `selectedpid` int(10) unsigned DEFAULT 0,
  `electiondata` datetime DEFAULT current_timestamp(),
  PRIMARY KEY (`pid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `monarch_election`
--

LOCK TABLES `monarch_election` WRITE;
/*!40000 ALTER TABLE `monarch_election` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `monarch_election` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `myshop_pricelist`
--

DROP TABLE IF EXISTS `myshop_pricelist`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `myshop_pricelist` (
  `owner_id` int(11) unsigned NOT NULL DEFAULT 0,
  `item_vnum` int(11) unsigned NOT NULL DEFAULT 0,
  `price` int(10) unsigned NOT NULL DEFAULT 0,
  UNIQUE KEY `list_id` (`owner_id`,`item_vnum`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `myshop_pricelist`
--

LOCK TABLES `myshop_pricelist` WRITE;
/*!40000 ALTER TABLE `myshop_pricelist` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `myshop_pricelist` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `object`
--

DROP TABLE IF EXISTS `object`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `object` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `land_id` int(11) unsigned NOT NULL DEFAULT 0,
  `vnum` int(10) unsigned NOT NULL DEFAULT 0,
  `map_index` int(11) unsigned NOT NULL DEFAULT 0,
  `x` int(11) NOT NULL DEFAULT 0,
  `y` int(11) NOT NULL DEFAULT 0,
  `x_rot` float NOT NULL DEFAULT 0,
  `y_rot` float NOT NULL DEFAULT 0,
  `z_rot` float NOT NULL DEFAULT 0,
  `life` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=63 DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `object`
--

LOCK TABLES `object` WRITE;
/*!40000 ALTER TABLE `object` DISABLE KEYS */;
set autocommit=0;
INSERT INTO `object` VALUES
(1,14,14100,1,434381,943692,0,0,357,0),
(2,214,14120,41,942433,296560,0,0,239,0),
(3,214,14014,41,942589,295616,0,0,268,0),
(4,14,14013,1,435010,943688,0,0,0,0),
(5,215,14120,41,926549,297088,0,0,153,0),
(6,215,14013,41,927683,297193,0,0,180,0),
(7,218,14120,41,934514,235591,0,0,91,0),
(8,218,14043,41,935080,236453,0,0,91,0),
(9,16,14100,1,432306,938129,0,0,91,0),
(10,16,14014,1,432023,938856,0,0,88,0),
(11,16,14043,1,432020,937381,0,0,88,0),
(12,108,14100,21,85379,150298,0,0,0,0),
(13,108,14014,21,84802,151960,0,0,306,0),
(14,214,14050,41,942620,294503,0,0,286,0),
(15,14,14051,1,435731,943698,0,0,0,0),
(16,215,14051,41,928980,297156,0,0,180,0),
(17,217,14100,41,930152,238176,0,0,0,0),
(18,218,14014,41,935077,234890,0,0,88,0),
(19,217,14015,41,932265,239139,0,0,106,0),
(20,109,14100,21,88293,163982,0,0,0,0),
(21,109,14051,21,88935,163932,0,0,0,0),
(22,17,14100,1,437106,934663,0,0,0,0),
(23,17,14015,1,436275,934657,0,0,0,0),
(24,207,14110,41,984568,272129,0,0,271,0),
(25,207,14014,41,982832,272694,0,0,239,0),
(26,15,14100,1,428350,943164,0,0,0,0),
(27,15,14015,1,429545,942613,0,0,0,0),
(28,217,14051,41,932282,237846,0,0,82,0),
(29,18,14110,1,428167,933318,0,0,88,0),
(30,18,14055,1,428940,934138,0,0,88,0),
(31,115,14120,21,39471,193625,0,0,88,0),
(32,115,14014,21,39485,192933,0,0,91,0),
(33,108,14043,21,85572,151991,0,0,321,0),
(34,18,14015,1,428931,932518,0,0,91,0),
(35,116,14120,21,36862,177326,0,0,88,0),
(36,116,14013,21,37371,176641,0,0,91,0),
(37,216,14110,41,934048,242407,0,0,88,0),
(38,109,14015,21,89730,164950,0,0,274,0),
(39,8,14120,1,491813,914323,0,0,0,0),
(40,216,14013,41,935180,241744,0,0,91,0),
(41,212,14100,41,939917,301675,0,0,0,0),
(42,117,14110,21,36840,173124,0,0,91,0),
(43,216,14055,41,935197,243190,0,0,88,0),
(44,117,14055,21,37747,173783,0,0,91,0),
(45,117,14014,21,37769,172444,0,0,88,0),
(46,205,14120,41,994899,268203,0,0,0,0),
(47,205,14055,41,993924,269178,0,0,268,0),
(48,15,14055,1,428969,942635,0,0,0,0),
(49,216,14200,41,935652,242478,0,0,91,0),
(50,216,14300,41,935789,240902,0,0,180,0),
(51,216,14300,41,935783,243771,0,0,180,0),
(52,205,14015,41,993944,267959,0,0,262,0),
(53,212,14015,41,939136,300886,0,0,286,0),
(54,206,14100,41,1000676,270395,0,0,321,0),
(55,206,14015,41,999041,271471,0,0,280,0),
(56,8,14015,1,492858,914619,0,0,94,0),
(57,115,14050,21,39428,192078,0,0,91,0),
(58,212,14055,41,939147,302549,0,0,256,0),
(59,207,14055,41,982795,273833,0,0,236,0),
(60,8,14055,1,493008,913361,0,0,88,0),
(61,208,14110,41,998798,274402,0,0,32,0),
(62,201,14100,41,989288,214730,0,0,0,0);
/*!40000 ALTER TABLE `object` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `pcbang_ip`
--

DROP TABLE IF EXISTS `pcbang_ip`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `pcbang_ip` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `pcbang_id` int(11) NOT NULL DEFAULT 0,
  `ip` varchar(15) NOT NULL DEFAULT '000.000.000.000',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE KEY `ip` (`ip`) USING BTREE,
  KEY `pcbang_id` (`pcbang_id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `pcbang_ip`
--

LOCK TABLES `pcbang_ip` WRITE;
/*!40000 ALTER TABLE `pcbang_ip` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `pcbang_ip` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `player`
--

DROP TABLE IF EXISTS `player`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `player` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `account_id` int(11) unsigned NOT NULL DEFAULT 0,
  `name` varchar(24) NOT NULL DEFAULT 'NONAME',
  `job` tinyint(2) unsigned NOT NULL DEFAULT 0,
  `voice` tinyint(1) NOT NULL DEFAULT 0,
  `dir` tinyint(2) NOT NULL DEFAULT 0,
  `x` int(11) NOT NULL DEFAULT 0,
  `y` int(11) NOT NULL DEFAULT 0,
  `z` int(11) NOT NULL DEFAULT 0,
  `map_index` int(11) NOT NULL DEFAULT 0,
  `exit_x` int(11) NOT NULL DEFAULT 0,
  `exit_y` int(11) NOT NULL DEFAULT 0,
  `exit_map_index` int(11) NOT NULL DEFAULT 0,
  `hp` int(11) NOT NULL DEFAULT 0,
  `mp` int(11) NOT NULL DEFAULT 0,
  `stamina` smallint(6) NOT NULL DEFAULT 0,
  `random_hp` smallint(5) NOT NULL DEFAULT 0 COMMENT 'if lvl 0, it will be negative',
  `random_sp` smallint(5) NOT NULL DEFAULT 0 COMMENT 'if lvl 0, it will be negative',
  `playtime` int(11) NOT NULL DEFAULT 0,
  `level` tinyint(2) unsigned NOT NULL DEFAULT 1,
  `level_step` tinyint(1) NOT NULL DEFAULT 0,
  `st` smallint(3) NOT NULL DEFAULT 0,
  `ht` smallint(3) NOT NULL DEFAULT 0,
  `dx` smallint(3) NOT NULL DEFAULT 0,
  `iq` smallint(3) NOT NULL DEFAULT 0,
  `exp` int(11) NOT NULL DEFAULT 0,
  `gold` bigint(19) NOT NULL DEFAULT 0,
  `cheque` int(11) NOT NULL DEFAULT 0,
  `stat_point` smallint(3) NOT NULL DEFAULT 0,
  `skill_point` smallint(3) NOT NULL DEFAULT 0,
  `quickslot` tinyblob DEFAULT NULL,
  `ip` varchar(15) DEFAULT '0.0.0.0',
  `part_main` int(10) unsigned NOT NULL DEFAULT 0,
  `part_base` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `part_hair` int(10) unsigned NOT NULL DEFAULT 0,
  `part_acce` int(11) unsigned NOT NULL DEFAULT 0,
  `skill_group` tinyint(4) NOT NULL DEFAULT 0,
  `skill_level` blob DEFAULT NULL,
  `alignment` int(11) NOT NULL DEFAULT 0,
  `last_play` datetime NOT NULL DEFAULT current_timestamp(),
  `change_name` tinyint(1) NOT NULL DEFAULT 0,
  `mobile` varchar(24) DEFAULT NULL,
  `sub_skill_point` smallint(3) NOT NULL DEFAULT 0,
  `stat_reset_count` tinyint(4) NOT NULL DEFAULT 0,
  `horse_hp` smallint(4) NOT NULL DEFAULT 0,
  `horse_stamina` smallint(4) NOT NULL DEFAULT 0,
  `horse_level` tinyint(2) unsigned NOT NULL DEFAULT 0,
  `horse_hp_droptime` int(10) unsigned NOT NULL DEFAULT 0,
  `horse_riding` tinyint(1) NOT NULL DEFAULT 0,
  `horse_skill_point` smallint(3) NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`) USING BTREE,
  KEY `account_id_idx` (`account_id`) USING BTREE,
  KEY `name_idx` (`name`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `player`
--

LOCK TABLES `player` WRITE;
/*!40000 ALTER TABLE `player` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `player` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `player_deleted`
--

DROP TABLE IF EXISTS `player_deleted`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `player_deleted` (
  `id` int(11) unsigned NOT NULL,
  `account_id` int(11) unsigned NOT NULL DEFAULT 0,
  `name` varchar(24) NOT NULL DEFAULT 'NONAME',
  `job` tinyint(2) unsigned NOT NULL DEFAULT 0,
  `voice` tinyint(1) NOT NULL DEFAULT 0,
  `dir` tinyint(2) NOT NULL DEFAULT 0,
  `x` int(11) NOT NULL DEFAULT 0,
  `y` int(11) NOT NULL DEFAULT 0,
  `z` int(11) NOT NULL DEFAULT 0,
  `map_index` int(11) NOT NULL DEFAULT 0,
  `exit_x` int(11) NOT NULL DEFAULT 0,
  `exit_y` int(11) NOT NULL DEFAULT 0,
  `exit_map_index` int(11) NOT NULL DEFAULT 0,
  `hp` int(11) NOT NULL DEFAULT 0,
  `mp` int(11) NOT NULL DEFAULT 0,
  `stamina` smallint(6) NOT NULL DEFAULT 0,
  `random_hp` smallint(5) NOT NULL DEFAULT 0 COMMENT 'if lvl 0, it will be negative',
  `random_sp` smallint(5) NOT NULL DEFAULT 0 COMMENT 'if lvl 0, it will be negative',
  `playtime` int(11) NOT NULL DEFAULT 0,
  `level` tinyint(2) unsigned NOT NULL DEFAULT 1,
  `level_step` tinyint(1) NOT NULL DEFAULT 0,
  `st` smallint(3) NOT NULL DEFAULT 0,
  `ht` smallint(3) NOT NULL DEFAULT 0,
  `dx` smallint(3) NOT NULL DEFAULT 0,
  `iq` smallint(3) NOT NULL DEFAULT 0,
  `exp` int(11) NOT NULL DEFAULT 0,
  `gold` bigint(19) NOT NULL DEFAULT 0,
  `cheque` int(11) NOT NULL DEFAULT 0,
  `stat_point` smallint(3) NOT NULL DEFAULT 0,
  `skill_point` smallint(3) NOT NULL DEFAULT 0,
  `quickslot` tinyblob DEFAULT NULL,
  `ip` varchar(15) DEFAULT '0.0.0.0',
  `part_main` int(10) unsigned NOT NULL DEFAULT 0,
  `part_base` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `part_hair` int(10) unsigned NOT NULL DEFAULT 0,
  `part_acce` int(11) unsigned NOT NULL DEFAULT 0,
  `skill_group` tinyint(4) NOT NULL DEFAULT 0,
  `skill_level` blob DEFAULT NULL,
  `alignment` int(11) NOT NULL DEFAULT 0,
  `last_play` datetime NOT NULL DEFAULT current_timestamp(),
  `change_name` tinyint(1) NOT NULL DEFAULT 0,
  `mobile` varchar(24) DEFAULT NULL,
  `sub_skill_point` smallint(3) NOT NULL DEFAULT 0,
  `stat_reset_count` tinyint(4) NOT NULL DEFAULT 0,
  `horse_hp` smallint(4) NOT NULL DEFAULT 0,
  `horse_stamina` smallint(4) NOT NULL DEFAULT 0,
  `horse_level` tinyint(2) unsigned NOT NULL DEFAULT 0,
  `horse_hp_droptime` int(10) unsigned NOT NULL DEFAULT 0,
  `horse_riding` tinyint(1) NOT NULL DEFAULT 0,
  `horse_skill_point` smallint(3) NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `player_deleted`
--

LOCK TABLES `player_deleted` WRITE;
/*!40000 ALTER TABLE `player_deleted` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `player_deleted` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `player_index`
--

DROP TABLE IF EXISTS `player_index`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `player_index` (
  `id` int(11) unsigned NOT NULL DEFAULT 0,
  `pid1` int(11) unsigned NOT NULL DEFAULT 0,
  `pid2` int(11) unsigned NOT NULL DEFAULT 0,
  `pid3` int(11) unsigned NOT NULL DEFAULT 0,
  `pid4` int(11) unsigned NOT NULL DEFAULT 0,
  `pid5` int(11) unsigned NOT NULL DEFAULT 0,
  `empire` tinyint(4) unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`) USING BTREE,
  KEY `pid1_key` (`pid1`) USING BTREE,
  KEY `pid2_key` (`pid2`) USING BTREE,
  KEY `pid3_key` (`pid3`) USING BTREE,
  KEY `pid4_key` (`pid4`) USING BTREE,
  KEY `pid5_key` (`pid5`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `player_index`
--

LOCK TABLES `player_index` WRITE;
/*!40000 ALTER TABLE `player_index` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `player_index` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `player_special_flag`
--

DROP TABLE IF EXISTS `player_special_flag`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `player_special_flag` (
  `pid` int(10) NOT NULL DEFAULT 0,
  `aid` int(10) NOT NULL DEFAULT 0,
  `flag` varchar(32) NOT NULL DEFAULT '',
  `value` bigint(18) NOT NULL DEFAULT 0,
  PRIMARY KEY (`pid`,`flag`,`aid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `player_special_flag`
--

LOCK TABLES `player_special_flag` WRITE;
/*!40000 ALTER TABLE `player_special_flag` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `player_special_flag` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `quest`
--

DROP TABLE IF EXISTS `quest`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `quest` (
  `dwPID` int(10) unsigned NOT NULL DEFAULT 0,
  `szName` varchar(32) NOT NULL DEFAULT '',
  `szState` varchar(64) NOT NULL DEFAULT '',
  `lValue` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`dwPID`,`szName`,`szState`) USING BTREE,
  KEY `pid_idx` (`dwPID`) USING BTREE,
  KEY `name_idx` (`szName`) USING BTREE,
  KEY `state_idx` (`szState`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `quest`
--

LOCK TABLES `quest` WRITE;
/*!40000 ALTER TABLE `quest` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `quest` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `safebox`
--

DROP TABLE IF EXISTS `safebox`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `safebox` (
  `account_id` int(10) unsigned NOT NULL DEFAULT 0,
  `size` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `password` varchar(6) NOT NULL DEFAULT '',
  `gold` bigint(19) NOT NULL DEFAULT 0,
  PRIMARY KEY (`account_id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `safebox`
--

LOCK TABLES `safebox` WRITE;
/*!40000 ALTER TABLE `safebox` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `safebox` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `sms_pool`
--

DROP TABLE IF EXISTS `sms_pool`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `sms_pool` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `server` int(11) NOT NULL DEFAULT 0,
  `sender` varchar(32) DEFAULT NULL,
  `receiver` varchar(100) NOT NULL DEFAULT '',
  `mobile` varchar(32) DEFAULT NULL,
  `sent` enum('N','Y') NOT NULL DEFAULT 'N',
  `msg` varchar(80) DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  KEY `sent_idx` (`sent`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `sms_pool`
--

LOCK TABLES `sms_pool` WRITE;
/*!40000 ALTER TABLE `sms_pool` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `sms_pool` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `special_spawn`
--

DROP TABLE IF EXISTS `special_spawn`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `special_spawn` (
  `vnum` int(11) NOT NULL,
  `channel` int(10) NOT NULL DEFAULT 0,
  `kill_time` datetime NOT NULL,
  PRIMARY KEY (`vnum`,`channel`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `special_spawn`
--

LOCK TABLES `special_spawn` WRITE;
/*!40000 ALTER TABLE `special_spawn` DISABLE KEYS */;
set autocommit=0;
INSERT INTO `special_spawn` VALUES
(1491,1,'2025-03-17 01:22:27'),
(1491,2,'2025-03-17 01:40:05'),
(1491,3,'2025-03-17 01:54:33'),
(1491,4,'2025-03-17 02:00:07'),
(1491,5,'2025-03-17 01:47:48'),
(1491,6,'2025-03-17 02:10:28'),
(1491,7,'2025-03-17 01:07:35'),
(1491,8,'2025-03-17 01:36:56'),
(2314,1,'2025-03-17 03:20:07'),
(2314,2,'2025-03-17 01:21:36'),
(2314,3,'2025-03-17 03:47:04'),
(2314,4,'2025-03-17 02:57:36'),
(2314,5,'2025-03-17 03:00:28'),
(2314,6,'2025-03-17 02:21:35'),
(2314,7,'2025-03-17 03:11:04'),
(2314,8,'2025-03-17 03:08:10');
/*!40000 ALTER TABLE `special_spawn` ENABLE KEYS */;
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
