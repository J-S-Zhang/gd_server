import '../config/server_config.dart';

class Constants {
  static String get serverHost => ServerConfig.host;
  static int get wsPort => ServerConfig.wsPort;
  static String get apiBaseUrl => ServerConfig.apiBaseUrl;
  static String get wsBaseUrl => ServerConfig.wsBaseUrl;
  static const int protocolVersion = 1;
  static const int heartbeatIntervalSeconds = 15;
  static const int turnTimeoutSeconds = 30;
  static const int playerCount = 6;
}
