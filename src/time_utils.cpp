#include "time_utils.h"

String getDuration(time_t diff) {
  long days = diff / 86400;
  long hours = (diff % 86400) / 3600;
  long minutes = (diff % 3600) / 60;
  long seconds = diff % 60;
  char buffer[64];
  if (days > 0) {
    snprintf(buffer, sizeof(buffer), "%ld д. %02ld:%02ld:%02ld", days, hours, minutes, seconds);
  } else {
    snprintf(buffer, sizeof(buffer), "%02ld:%02ld:%02ld", hours, minutes, seconds);
  }
  return String(buffer);
}

String getFormattedTime(time_t t) {
  struct tm timeinfo;
  localtime_r(&t, &timeinfo);
  char buffer[32];
  strftime(buffer, sizeof(buffer), "%H:%M:%S %d.%m.%Y", &timeinfo);
  return String(buffer);
}

void setupNtpTime() {
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
  tzset();

  Serial.print("Waiting for NTP sync");
  time_t now = time(nullptr);
  while (now < 1000000) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\nTime synchronized");
}
