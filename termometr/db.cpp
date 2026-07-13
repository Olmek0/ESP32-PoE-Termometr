#include "db_functions.h"
#include "web_functions.h"
#include "general_functions.h"
// baza danych, tworzona jeśli nie istnieje

void initDatabase() {
  String dbPath = "/sdcard/temperature.db";

  if (sqlite3_open("/sdcard/temperature.db", &db)) {
    Serial.printf("[DB ERROR] %s\n", sqlite3_errmsg(db));
    return;
  }

  // Tworzenie tabeli
  const char *create_table_sql = R"sql(
    CREATE TABLE IF NOT EXISTS logs2 (
      timestamp TEXT NOT NULL,
      temperature_c REAL NOT NULL,
      temperature_f REAL NOT NULL
    );
  )sql";

  // inicjalizacja
  int rc = sqlite3_exec(db, create_table_sql, NULL, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    Serial.printf("[DB ERROR] SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  } else {
    Serial.println("[DB] Table ready");
  }

  sqlite3_close(db);
}

// zapisz temp do bazy danych

void logTemperature(float tempC, float tempF, const String& timestamp) {
  if (sqlite3_open("/sdcard/temperature.db", &db)) {
    Serial.println("[DB ERROR] Failed to open database");
    return;
  }

  const char* query = "INSERT INTO logs2 (timestamp, temperature_c, temperature_f) VALUES (?, ?, ?);";

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("[DB ERROR] Prepare failed: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db); // Always close the DB if prepare fails!
    return;
  }

  sqlite3_bind_text(stmt, 1, timestamp.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt, 2, static_cast<double>(tempC));
  sqlite3_bind_double(stmt, 3, static_cast<double>(tempF));

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) { // Dla INSERT/UPDATE musi być DONE a nie ROW
    Serial.printf("[DB ERROR] Insert step failed: %s\n", sqlite3_errmsg(db));
  } else {
    Serial.println("[DB] Logged safely: " + timestamp + ", " + String(tempC) + "°C");
  }
  sqlite3_finalize(stmt);
  sqlite3_close(db);
}

// wysyłanie statystyk

void sendStatsOverWebSocket() {
  if (sqlite3_open("/sdcard/temperature.db", &db)) {
    Serial.println("[DB ERROR] Failed to open database for stats");
    return;
  }
  const char* Query = R"sql(
    SELECT 
      COUNT(*) AS total,
      MAX(temperature_c) AS max_temp,
      MIN(temperature_c) AS min_temp,
      MAX(temperature_f) AS max_tempf,
      MIN(temperature_f) AS min_tempf,
      DATE(MIN(timestamp)) AS first_entry
    FROM logs2;                                                                       
  )sql";

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, Query, -1, &stmt, NULL);

  if (rc != SQLITE_OK) {
    Serial.printf("[DB ERROR] Query failed: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return;
  }

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    int total = sqlite3_column_int(stmt, 0);
    float maxTemp = sqlite3_column_double(stmt, 1);
    float minTemp = sqlite3_column_double(stmt, 2);
    float maxTempf = sqlite3_column_double(stmt, 3);
    float minTempf = sqlite3_column_double(stmt, 4);
    const unsigned char* firstEntry = sqlite3_column_text(stmt, 5);

    String start = firstEntry ? String((const char*)firstEntry) : "unknown";

    String statsJson = "{\"total\":" + String(total) + ",\"max\":" + String(maxTemp) + ",\"min\":" + String(minTemp) + ",\"maxf\":" + String(maxTempf) + ",\"minf\":" + String(minTempf)
    + ",\"start\":\"" + start + "\"}";

    webSocket.broadcastTXT(statsJson);
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);

}

// Wyszukiwanie historii

void sendHistoryJson(const String& start, const String& end) {
  if (sqlite3_open("/sdcard/temperature.db", &db)) {
    server.send(500, "application/json", "{\"error\":\"Failed to open database\"}");
    return;
  }

  const char* query = "SELECT timestamp, temperature_c, temperature_f FROM logs2 "
                 "WHERE DATE(timestamp) >= DATE(?) AND DATE(timestamp) <= DATE(?) "
                 "ORDER BY timestamp ASC;";

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("[DB ERROR] Prepare failed: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    server.send(500, "application/json", "{\"error\":\"Failed to execute query\"}");
    return;
  }
  sqlite3_bind_text(stmt, 1, start.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, end.c_str(), -1, SQLITE_TRANSIENT);

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");

  server.sendContent("{ \"total\":0, \"data\":[");

  bool first = true;
  int total = 0;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    if (!first) server.sendContent(",");
    first = false;
    total++;

    const char* ts = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    float c = sqlite3_column_double(stmt, 1);
    float f = sqlite3_column_double(stmt, 2);

    String row = "{\"timestamp\":\"" + String(ts) + "\",\"c\":" + String(c, 2) + ",\"f\":" + String(f, 2) + "}";
    server.sendContent(row);
  }

  server.sendContent("], \"total\":" + String(total) + " }");

  sqlite3_finalize(stmt);
  sqlite3_close(db);

}

// Wysyłanie informacji o wykresie

void sendChartData() {
  if (sqlite3_open("/sdcard/temperature.db", &db)) {
    Serial.println("[DB ERROR] Cannot open DB for chart");
    return;
  }

  // Grupowanie dla godzin
  const char *query24h = R"sql(
    SELECT 
      strftime('%Y-%m-%d %H:00', timestamp) AS hour,
      AVG(temperature_c) AS avg_c,
      AVG(temperature_f) AS avg_f
    FROM logs2
    WHERE timestamp >= datetime('now', '-24 hours')
    GROUP BY hour
    ORDER BY hour ASC;
  )sql";

  // Grupowanie dla dni
  const char *query30d = R"sql(
    SELECT 
      strftime('%Y-%m-%d', timestamp) AS day,
      AVG(temperature_c) AS avg_c,
      AVG(temperature_f) AS avg_f
    FROM logs2
    WHERE timestamp >= date('now', '-30 days')
    GROUP BY day
    ORDER BY day ASC;
  )sql";

  String json = "{\"type\":\"chart\"";

  // Ostatnie 24 godziny
  sqlite3_stmt *stmt24;
  if (sqlite3_prepare_v2(db, query24h, -1, &stmt24, NULL) == SQLITE_OK) {
    json += ",\"data\":[";
    bool first = true;
    while (sqlite3_step(stmt24) == SQLITE_ROW) {
      if (!first) json += ",";
      first = false;

      const char *hour = reinterpret_cast<const char *>(sqlite3_column_text(stmt24, 0));
      float avg_c = sqlite3_column_double(stmt24, 1);
      float avg_f = sqlite3_column_double(stmt24, 2);

      json += "{\"hour\":\"" + String(hour) + "\",\"avg_c\":" + String(avg_c, 2) + ",\"avg_f\":" + String(avg_f, 2) + "}";
    }
    json += "]";
    sqlite3_finalize(stmt24);
  } else {
    Serial.printf("[DB ERROR] 24h query failed: %s\n", sqlite3_errmsg(db));
  }

  // Ostatnie 30 dni
  sqlite3_stmt *stmt30;
  if (sqlite3_prepare_v2(db, query30d, -1, &stmt30, NULL) == SQLITE_OK) {
    json += ",\"monthly\":[";
    bool first = true;
    while (sqlite3_step(stmt30) == SQLITE_ROW) {
      if (!first) json += ",";
      first = false;

      const char *day = reinterpret_cast<const char *>(sqlite3_column_text(stmt30, 0));
      float avg_c = sqlite3_column_double(stmt30, 1);
      float avg_f = sqlite3_column_double(stmt30, 2);

      json += "{\"day\":\"" + String(day) + "\",\"avg_c\":" + String(avg_c, 2) + ",\"avg_f\":" + String(avg_f, 2) + "}";
    }
    json += "]";
    sqlite3_finalize(stmt30);
  } else {
    Serial.printf("[DB ERROR] 30d query failed: %s\n", sqlite3_errmsg(db));
  }

  json += "}";

  sqlite3_close(db);

  webSocket.broadcastTXT(json);
}
