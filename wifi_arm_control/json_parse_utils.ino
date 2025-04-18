//json verisini kontrol eder ve hareketi başlatır
bool parseAndMove(const String& json) {
  StaticJsonDocument<16384> doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    Serial.print("JSON parse hatası: ");
    Serial.println(error.f_str());
    return false;
  }

  JsonArray commands = doc["commands"];
  if (commands.isNull() || commands.size() == 0) {
    Serial.println("Hata: 'commands' eksik veya bozuk!");
    return false;
  }

  currentCommands.clear();

  for (JsonArray command : commands) {
    if (command.size() != 6) {
      Serial.println("Hata: Komut dizisi 6 eleman içermiyor!");
      return false;
    }

    std::array<int, 6> angles;

    for (int j = 0; j < 6; j++) {
      if (!command[j].is<int>()) {
        Serial.println("Hata: Komut içindeki bir öğe geçersiz!");
        return false;
      }
      int angle = command[j].as<int>();
      // if (angle < 0 || angle > 181) {
      //   Serial.println("Hata: Servo açısı sınırların dışında!");
      //   return false;
      // }
      if (angle < 544 || angle > 2400) {
        Serial.println("Hata: Servo açısı sınırların dışında!");
        return false;
      }
      angles[j] = angle;
    }

    currentCommands.push_back(angles);
  }

   currentCommandIndex = 0;
   isMoving = true;

  Serial.println("Komutlar başarıyla yüklendi.");
  return true;
}