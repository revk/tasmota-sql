-- MariaDB dump 10.19  Distrib 10.5.12-MariaDB, for debian-linux-gnu (x86_64)
--
-- Host: localhost    Database: tasmota
-- ------------------------------------------------------
-- Server version	10.5.12-MariaDB-0+deb11u1

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `device`
--

DROP TABLE IF EXISTS `device`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `device` (
  `Topic` varchar(100) NOT NULL COMMENT 'The main device topic, unique key for this table',
  `MqttHost` tinytext DEFAULT NULL COMMENT 'MQTT Hostname',
  `Module` int(11) DEFAULT NULL COMMENT 'Module type code',
  `SSId1` tinytext DEFAULT NULL COMMENT 'SSID',
  `SSId2` tinytext DEFAULT NULL COMMENT 'SSID',
  `Hostname` tinytext DEFAULT NULL COMMENT 'Hostname used on DHCP',
  `DeviceName` tinytext DEFAULT NULL COMMENT 'Device name',
  `MqttClient` tinytext DEFAULT NULL COMMENT 'MQTT client ID',
  `Webserver` enum('0','1','2') DEFAULT NULL COMMENT 'If webserver enabled',
  `Sleep` int(11) DEFAULT NULL COMMENT 'Sleep (ms)',
  `GroupTopic1` tinytext DEFAULT NULL COMMENT 'Group topic',
  `GroupTopic2` tinytext DEFAULT NULL COMMENT 'Group topic',
  `GroupTopic3` tinytext DEFAULT NULL COMMENT 'Group topic',
  `GroupTopic4` tinytext DEFAULT NULL COMMENT 'Group topic',
  `Rule1` tinytext DEFAULT NULL COMMENT 'Rules',
  `Rule2` tinytext DEFAULT NULL COMMENT 'Rules',
  `Rule3` tinytext DEFAULT NULL COMMENT 'Rules',
  `SwitchMode1` enum('0','1','2','3','4','5','6','7','8','9','10','11','12','13','14','15') DEFAULT NULL COMMENT 'Switch mode code',
  `SwitchText1` tinytext DEFAULT NULL COMMENT 'Switch text',
  `PulseTime1` int(11) DEFAULT NULL COMMENT 'Switch pulse time',
  `WebButton1` tinytext DEFAULT NULL COMMENT 'Switch web button',
  `FriendlyName1` tinytext DEFAULT NULL COMMENT 'Switch friendly name',
  `SwitchMode2` enum('0','1','2','3','4','5','6','7','8','9','10','11','12','13','14','15') DEFAULT NULL COMMENT 'Switch mode code',
  `SwitchText2` tinytext DEFAULT NULL COMMENT 'Switch text',
  `PulseTime2` int(11) DEFAULT NULL COMMENT 'Switch pulse time',
  `WebButton2` tinytext DEFAULT NULL COMMENT 'Switch web button',
  `FriendlyName2` tinytext DEFAULT NULL COMMENT 'Switch friendly name',
  `SwitchMode3` enum('0','1','2','3','4','5','6','7','8','9','10','11','12','13','14','15') DEFAULT NULL COMMENT 'Switch mode code',
  `SwitchText3` tinytext DEFAULT NULL COMMENT 'Switch text',
  `PulseTime3` int(11) DEFAULT NULL COMMENT 'Switch pulse time',
  `WebButton3` tinytext DEFAULT NULL COMMENT 'Switch web button',
  `FriendlyName3` tinytext DEFAULT NULL COMMENT 'Switch friendly name',
  `SwitchMode4` enum('0','1','2','3','4','5','6','7','8','9','10','11','12','13','14','15') DEFAULT NULL COMMENT 'Switch mode code',
  `SwitchText4` tinytext DEFAULT NULL COMMENT 'Switch text',
  `PulseTime4` int(11) DEFAULT NULL COMMENT 'Switch pulse time',
  `WebButton4` tinytext DEFAULT NULL COMMENT 'Switch web button',
  `FriendlyName4` tinytext DEFAULT NULL COMMENT 'Switch friendly name',
  `NtpServer1` tinytext DEFAULT NULL COMMENT 'NTP server',
  `NtpServer2` tinytext DEFAULT NULL COMMENT 'NTP server',
  `NtpServer3` tinytext DEFAULT NULL COMMENT 'NTP server',
  `Latitude` decimal(10,6) DEFAULT NULL COMMENT 'Latitude',
  `Longitude` decimal(11,6) DEFAULT NULL COMMENT 'Longitude',
  `BlinkTime` int(11) DEFAULT NULL COMMENT 'Blink Time',
  `BlinkCount` int(11) DEFAULT NULL COMMENT 'Blink Count',
  `WebLog` enum('0','1','2','3','4') DEFAULT NULL COMMENT 'Web logging',
  `SetOption0` enum('0','1') DEFAULT NULL COMMENT 'Save power state and use after restart',
  `SetOption1` enum('0','1') DEFAULT NULL COMMENT 'Restrict multipress options',
  `SetOption3` enum('0','1') DEFAULT NULL COMMENT 'Enable MQTT',
  `SetOption4` enum('0','1') DEFAULT NULL COMMENT 'MQTT response type',
  `SetOption8` enum('0','1') DEFAULT NULL COMMENT 'Temperature 0=C/1=F',
  `SetOption10` enum('0','1') DEFAULT NULL COMMENT 'MQTT topic handling',
  `SetOption11` enum('0','1') DEFAULT NULL COMMENT 'Swap single/double press',
  `SetOption12` enum('0','1') DEFAULT NULL COMMENT 'Fixed flash slot',
  `SetOption13` enum('0','1') DEFAULT NULL COMMENT 'Allow simple button immediate',
  `SetOption15` enum('0','1') DEFAULT NULL COMMENT 'Set PWM control for LED',
  `SetOption16` enum('0','1') DEFAULT NULL COMMENT 'LED Clock scheme',
  `SetOption18` enum('0','1') DEFAULT NULL COMMENT 'Light paired with CO2',
  `SetOption19` enum('0','1') DEFAULT NULL COMMENT 'Home assistant protocol',
  `SetOption20` enum('0','1') DEFAULT NULL COMMENT 'Dimmer/Color/CT without power on',
  `SetOption21` enum('0','1') DEFAULT NULL COMMENT 'Energy monitor when power off',
  `SetOption24` enum('0','1') DEFAULT NULL COMMENT 'Pressure 0=hPA/1=mmHg',
  `SetOption26` enum('0','1') DEFAULT NULL COMMENT 'Use index even when only one relay',
  `SetOption28` enum('0','1') DEFAULT NULL COMMENT 'RF data format',
  `SetOption29` enum('0','1') DEFAULT NULL COMMENT 'IR data format',
  `SetOption30` enum('0','1') DEFAULT NULL COMMENT 'Enforce HA auto as light',
  `SetOption31` enum('0','1') DEFAULT NULL COMMENT 'LED blink during wifi/mqtt issues',
  `SetOption32` int(3) DEFAULT NULL COMMENT 'Hold time (0.1s)',
  `SetOption33` int(3) DEFAULT NULL COMMENT 'Max power limit (s)',
  `SetOption34` int(3) DEFAULT NULL COMMENT 'Backlog delay (ms)',
  `SetOption36` int(3) DEFAULT NULL COMMENT 'Boot loops',
  `SetOption38` int(3) DEFAULT NULL COMMENT 'IRReceive protocol detection',
  `SetOption39` int(3) DEFAULT NULL COMMENT 'Invalid power handling',
  `SetOption40` int(3) DEFAULT NULL COMMENT 'Stop detection input change no button GPIO',
  `SetOption41` int(3) DEFAULT NULL COMMENT 'ARP keepalive (s)',
  `SetOption42` int(3) DEFAULT NULL COMMENT 'Over temp threshold (C)',
  `SetOption43` int(3) DEFAULT NULL COMMENT 'Rotary step',
  `SetOption51` enum('0','1') DEFAULT NULL COMMENT 'GPIO9/10 enable (bad)',
  `SetOption52` enum('0','1') DEFAULT NULL COMMENT 'Time offset in JSON',
  `SetOption53` enum('0','1') DEFAULT NULL COMMENT 'Hostname/IP in GUI',
  `SetOption54` enum('0','1') DEFAULT NULL COMMENT 'Apply option 20 to Tuya',
  `SetOption55` enum('0','1') DEFAULT NULL COMMENT 'mDNS enable',
  `SetOption56` enum('0','1') DEFAULT NULL COMMENT 'WiFi scan on start',
  `SetOption57` enum('0','1') DEFAULT NULL COMMENT 'WiFi scan evert 44 minutes',
  `SetOption58` enum('0','1') DEFAULT NULL COMMENT 'IR raw data in JSON',
  `SetOption60` enum('0','1') DEFAULT NULL COMMENT 'Sleep mode normal',
  `SetOption61` enum('0','1') DEFAULT NULL COMMENT 'Force local button/switch',
  `SetOption62` enum('0','1') DEFAULT NULL COMMENT 'Set retain on button/switch',
  `SetOption63` enum('0','1') DEFAULT NULL COMMENT 'Relay feedback scan at start',
  `SetOption64` enum('0','1') DEFAULT NULL COMMENT 'Switch -/_ on sensor name',
  `SetOption65` enum('0','1') DEFAULT NULL COMMENT 'Device recover fast cycle detect',
  `SetOption66` enum('0','1') DEFAULT NULL COMMENT 'Publish TuyaReceive to MQTT',
  `SetOption71` enum('0','1') DEFAULT NULL COMMENT 'DDS238 Modbus register',
  `SetOption72` enum('0','1') DEFAULT NULL COMMENT 'Use energy monitor hardware',
  `SetOption73` enum('0','1') DEFAULT NULL COMMENT 'Detach buttons from relays',
  `SetOption74` enum('0','1') DEFAULT NULL COMMENT 'Enable pullup on DS18x20',
  `SetOption75` enum('0','1') DEFAULT NULL COMMENT 'Grouptopic behaviour',
  `SetOption76` enum('0','1') DEFAULT NULL COMMENT 'Bootcount in deepsleep',
  `SetOption77` enum('0','1') DEFAULT NULL COMMENT 'Do not power off if slider moved to far left',
  `SetOption78` enum('0','1') DEFAULT NULL COMMENT 'Reset counters at TelePeriod time',
  `SetOption80` enum('0','1') DEFAULT NULL COMMENT 'Blinds and shutters',
  `SetOption81` enum('0','1') DEFAULT NULL COMMENT 'PCF8574 inverted',
  `SetOption82` enum('0','1') DEFAULT NULL COMMENT 'CT ranges higher',
  `SetOption83` enum('0','1') DEFAULT NULL COMMENT 'Zibgee device friendly name',
  `SetOption85` enum('0','1') DEFAULT NULL COMMENT 'Device group support',
  `SetOption86` enum('0','1') DEFAULT NULL COMMENT 'Turn off brightness LEDs 5 seconds after last change',
  `SetOption87` enum('0','1') DEFAULT NULL COMMENT 'Turn red LED on when powered off',
  `SetOption88` enum('0','1') DEFAULT NULL COMMENT 'Make relays part of separate device group',
  `SetOption90` enum('0','1') DEFAULT NULL COMMENT 'Disable MQTT with no JSON messages',
  `SetOption93` enum('0','1') DEFAULT NULL COMMENT 'Caching of compressed rules',
  `SetOption94` enum('0','1') DEFAULT NULL COMMENT 'MAX6675 for MAX31855',
  `SetOption97` enum('0','1') DEFAULT NULL COMMENT 'TiyaMCU baud 0=9600/1=115200',
  `SetOption98` enum('0','1') DEFAULT NULL COMMENT 'Rotary dimmer rule triggers',
  `SetOption99` enum('0','1') DEFAULT NULL COMMENT 'Zero cross AC dimmer attached',
  `SetOption101` enum('0','1') DEFAULT NULL COMMENT 'Zigbee source endpoints as suffix',
  `SetOption103` enum('0','1') DEFAULT NULL COMMENT 'Enable TLS',
  `SetOption104` enum('0','1') DEFAULT NULL COMMENT 'Disable MQTT retained messages',
  `SetOption107` enum('0','1') DEFAULT NULL COMMENT 'Cirtual CT channel 0=warm/1=cold',
  `SetOption108` enum('0','1') DEFAULT NULL COMMENT 'Teleinfo each frame',
  `SetOption109` enum('0','1') DEFAULT NULL COMMENT 'Force gen1 Alexa mode',
  `SetOption113` enum('0','1') DEFAULT NULL COMMENT 'Set dimmer low on rotary dial after power off',
  `SetOption114` enum('0','1') DEFAULT NULL COMMENT 'Detach switches from relays and send MQTT instead',
  `SetOption115` enum('0','1') DEFAULT NULL COMMENT 'ESP32 MI32 BLE enable',
  `SetOption116` enum('0','1') DEFAULT NULL COMMENT 'Auto query lights/devices',
  `SetOption117` enum('0','1') DEFAULT NULL COMMENT 'Fade at fixed duration',
  `SetOption123` enum('0','1') DEFAULT NULL COMMENT 'Wiegand tag number output in hex',
  `SetOption124` enum('0','1') DEFAULT NULL COMMENT 'Wiegand key pad stroke format',
  `SetOption125` enum('0','1') DEFAULT NULL COMMENT 'ZbBridge hide',
  `SetOption126` enum('0','1') DEFAULT NULL COMMENT 'Arithmetic mean over teleperiod for JSON with DS18x20',
  `SetOption127` enum('0','1') DEFAULT NULL COMMENT 'Wifi no sleep',
  PRIMARY KEY (`Topic`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
/*!40101 SET character_set_client = @saved_cs_client */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2021-10-26  9:14:27
