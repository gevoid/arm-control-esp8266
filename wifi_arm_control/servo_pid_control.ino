// pid zamanı gelmiş mi bakar , gerekli hesaplamaları ve komutları yürütür
unsigned long lastCommandIndexIncrease = 0;
void pidCheck(unsigned long now){
   if (now - lastPIDTime >= pidInterval) {
    lastPIDTime = now;

    if (isMoving) {
      if (currentCommandIndex < currentCommands.size()) {
        std::array<int, 6> currentCommand = currentCommands[currentCommandIndex];

        // Hedef açıları güncelle
        for (int i = 0; i < 6; i++) {
          servoTargetAnglesMicroSeconds[i] = currentCommand[i];
        }

        // Tüm servolar hedefe ulaştıysa sıradakine geç
        bool allReached = false;

        if (areServosAtTarget()) {
          Serial.println("tüm servolar hedefe ulaştı komut: " + String(currentCommandIndex));
          allReached = true;
        }

        updateServosWithPIDMicros();
        if (allReached) {
          if(now-lastCommandIndexIncrease >= 200){
            lastCommandIndexIncrease = now;
            currentCommandIndex++;
          }
          
          // delay(200);  // geçiş efekti
        }

      } else {
        isMoving = false;
        currentCommandIndex = 0;
        resetServoPID();
        Serial.println("Tüm komutlar tamamlandı.");
      }
    }
  }
}

//servoların mikrosaniye cinsinden hedeflerini hesaplar
float calculatePIDMicros(int i) {
  float error = servoTargetAnglesMicroSeconds[i] - servoAnglesMicroSeconds[i];

  if (abs(error) < 2) {
    integrals[i] = 0;
    return 0;
  }

  integrals[i] += error * dt;
  integrals[i] = constrain(integrals[i], -500, 500);

  float derivative = (error - prevErrors[i]) / dt;
  prevErrors[i] = error;

  float output = servoPID[i].kp * error +
                 servoPID[i].ki * integrals[i] +
                 servoPID[i].kd * derivative;

  output = constrain(output, -20.0, 20.0); // küçük adımlar

  return output;
}

// tüm servolar için heaplamaları yapar
void updateServosWithPIDMicros() {
  for (int i = 0; i < 6; i++) {
    float output = calculatePIDMicros(i);
    float newMicros = servoAnglesMicroSeconds[i] + output;
    newMicros = constrain(newMicros, SERVO_MIN_PULSE, SERVO_MAX_PULSE);

    if (abs(newMicros - servoAnglesMicroSeconds[i]) > 1) {
      servoAnglesMicroSeconds[i] = newMicros;
      // gripper için açı koruması
      if(i == 0){
        if(servoAnglesMicroSeconds[i] < 1781) servoAnglesMicroSeconds[i] =1781;
        if(servoAnglesMicroSeconds[i] > 2400) servoAnglesMicroSeconds[i] =2400;
      }
      servos[i].writeMicroseconds(static_cast<int>(servoAnglesMicroSeconds[i]));
    }
  }
}

// servolar hedef açılarda mı kontrol eder
bool areServosAtTarget() {
  for (int i = 0; i < 6; i++) {
    if (abs(servoTargetAnglesMicroSeconds[i] - servoAnglesMicroSeconds[i]) > 2) {
      Serial.println("Servo: "+String(i+1) +" Hedefe ulaşamıyor");
      return false;
    }
  }
  return true;
}

// tüm işlemler bittikten sonra pid hatalarını ve integrallerini temizler
void resetServoPID() {
  for (int i = 0; i < 6; i++) {
        integrals[i] = 0;
        prevErrors[i] = 0;
      }
}