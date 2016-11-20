#define SSIDBASE 200
#define PASSWORDBASE 220
#define MAGIC_BYTE 0x55

#define firmware "iotupdater"
#define firmware_version firmware"_001"

#define update_server "lindmark.synology.me"
#define update_uri "/esp/update/espupdater.php"

bool readCredentials() {
  
  EEPROM.begin(512);
    
  if (EEPROM.read(SSIDBASE - 1) != MAGIC_BYTE)  {
    Serial.println(EEPROM.read(SSIDBASE - 1), HEX);

    for (int i = 0; ssid[i] != 0; EEPROM.write(SSIDBASE + i, ssid[i]),  i++);
    for (int i = 0; password[i] != 0; EEPROM.write(PASSWORDBASE + i, password[i]),  i++);
    
    EEPROM.write(SSIDBASE - 1, MAGIC_BYTE);

    Serial.println("ssid stored");  
  }

  int i = 0;
  while(ssid[i] = EEPROM.read(SSIDBASE + i) && i < sizeof(ssid)){
    i++;
  }
   
  i = 0;
  while(password[i] = EEPROM.read(PASSWORDBASE + i) && i < sizeof(password)){
    i++;
  } 

  Serial.println("ssid stored");
  
  EEPROM.end();
}

void printMacAddress() {
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(mac[i], HEX);
    Serial.print(":");
  }
  Serial.println(mac[5], HEX);
}


void iotUpdater(bool debug) {
  if (debug) {
    printMacAddress();
    Serial.println("start flashing......");
    Serial.println(update_server);
    Serial.println(update_uri);
    Serial.println(firmware_version);
  }

  t_httpUpdate_return ret = ESPhttpUpdate.update(update_server, 80, update_uri, firmware_version);
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      if (debug) Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      if (debug) Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      if (debug) Serial.println("HTTP_UPDATE_OK");
      break;
  }
}

