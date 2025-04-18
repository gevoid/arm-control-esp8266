// servoların nihai hareketleri buradan gerçekleşir ve açıları kayıt edilir
void moveServo(int servoIndex, int targetAngle) {
  // gripper açı koruması
  if (servoIndex == 0) {
    if (targetAngle > 180) targetAngle = 180;
    if (targetAngle < 120) targetAngle = 120;
  }
  servos[servoIndex].write(targetAngle);
  servoAngles[servoIndex] = targetAngle;
  servoAnglesMicroSeconds[servoIndex] = servos[servoIndex].readMicroseconds();
}

// servoları ivmeli hareket ettirir ve güncel açılarını kaydeder sadece servoyu api ucundan hareket ettirmek için kullanılır
void moveServoSmooth(int servoNum, int targetAngle) {
  int currentAngle = servos[servoNum - 1].read();
  while (currentAngle != targetAngle) {
    // İvmeli hareket
    if (currentAngle < targetAngle) {
      speed += acceleration;
      currentAngle += speed;
      if (currentAngle > targetAngle) currentAngle = targetAngle;  // Hedefi geçme
    } else {
      speed += acceleration;
      currentAngle -= speed;
      if (currentAngle < targetAngle) currentAngle = targetAngle;  // Hedefi geçme
    }
    moveServo(servoNum - 1, currentAngle);

    delay(20);  // Hareketi yumuşatmak için küçük bir bekleme
  }
  servoAngles[servoNum - 1] = targetAngle;
  speed = 1.0;  // Yeni hareket için hızı sıfırla
}

// increment servo move 
void ism(int servoIndex, int step) {
  servoAnglesMicroSeconds[servoIndex] += step;
  if (servoAnglesMicroSeconds[servoIndex] > 2400) servoAnglesMicroSeconds[servoIndex] = 2400;
  servos[servoIndex].writeMicroseconds(servoAnglesMicroSeconds[servoIndex]);
}

// decrement servo move 
void dsm(int servoIndex, int step) {
  servoAnglesMicroSeconds[servoIndex] -= step;
  // gripper açı koruması
  if (servoIndex == 0 && servoAnglesMicroSeconds[servoIndex] < 1781) {
    servoAnglesMicroSeconds[servoIndex] = 1781;
  }
  if (servoAnglesMicroSeconds[servoIndex] < 544) servoAnglesMicroSeconds[servoIndex] = 544;
  servos[servoIndex].writeMicroseconds(servoAnglesMicroSeconds[servoIndex]);
}

// manuel kontrollerde hala veri geliyorsa basılı tutuluyor mu kontrolu yapar ve ne kadar çok basıldıysa açıyı arttıtır
void manuelControlCheck(unsigned long now){
  for (int i = 0; i < 6; i++) {
    if (desiredDirection[i] != 0 && now - lastPressTime[i] > pressTimeout) {
      // tuş bırakıldı
      desiredDirection[i] = 0;
      pressStartTime[i] = 0;
    }

    if (desiredDirection[i] != 0) {
      if (pressStartTime[i] == 0) pressStartTime[i] = now;

      unsigned long pressDuration = now - pressStartTime[i];
      int stepSize = map(pressDuration, 0, 2000, 2, 10);  // 2-10 µs
      stepSize = constrain(stepSize, 1, 15);

      if (desiredDirection[i] == 1) {
        ism(i, stepSize);
      } else if (desiredDirection[i] == -1) {
        dsm(i, stepSize);
      }
    }
  }
}

