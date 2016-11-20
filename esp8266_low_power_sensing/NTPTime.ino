bool adjust_time(NTPClient *timeClient) {


  int i = 10;
  while (!timeClient->forceUpdate() && i > 0) {
    delay(500);
    i--;
  }

  if (i == 0) {
    return false;
  }


  Serial.println(timeClient->getFormattedTime());

  int hour = timeClient->getHours();
  int minute = timeClient->getMinutes();
  int second = timeClient->getSeconds();

  // Calculate hours to status report and update counter
  rtcMem.counter = STATUS_HOUR - hour;
  if (rtcMem.counter <= 0) {
    rtcMem.counter += HOURS_DAY;
  }
  system_rtc_mem_write(RTCMEMORYSTART, &rtcMem, sizeof(rtcStore));


  // Time until next full hour
  uint32 adjusted_time = ( (59 - minute) * 60 + (60 - second)) * SECOND; // 8:00:15 -> 59:45 min:sec;  8:59:59 -> 1 sec

  char count_str[10];

  sprintf (count_str, "%ld", (rtcMem.counter - 1) );
  Serial.println("");
  Serial.print(count_str);
  Serial.print(" Hours and ");
  sprintf (count_str, "%u", (adjusted_time / SECOND / 60));
  Serial.print(count_str);
  Serial.println(" Minutes until Status event");

  // Skip reporting if I reported lass than 1 minute ago
  if ( (rtcMem.counter - 1) == 0 && minute == 0) {
    sleep_no_rf(adjusted_time + HOUR);
  }

  return true;
}
