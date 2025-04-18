// mesafe sensörü, sıcaklık sensörü okuma ve hesaplamaları
void readSensors(unsigned long now){
      if (now - lastTempRequest > 2000) {
    if (!tempRequested) {
      sensors.requestTemperatures();
      tempRequested = true;
      lastTempRequest = now;
    } else {
      double temporaryValue = sensors.getTempCByIndex(0);
      if (temporaryValue > -10)  tempValue = temporaryValue;
      digitalWrite(fanPowerTrigPin, tempValue > 30 ? HIGH : LOW);
      tempRequested = false;
    }
  }
  if (now - lastDistanceRead >= 500) {
    double tempDistanceValue =sonar.ping_cm();
    lastDistanceRead = now;
    if (tempDistanceValue != 0)   distanceValue = tempDistanceValue;
  }
}


