const char *pidControlHTML = R"rawliteral(
<!DOCTYPE html>
<html lang="tr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>PID Ayarları - Servo Kontrol</title>
  <style>
    body { font-family: Arial, sans-serif; background: #fafafa; margin: 0; padding: 20px; }
    h1 { text-align: center; color: #333; }
    .container { max-width: 800px; margin: 0 auto; padding: 20px; background: #fff; border-radius: 8px; box-shadow: 0 4px 10px rgba(0, 0, 0, 0.1); }
    table { width: 100%; margin-top: 20px; border-collapse: collapse; }
    th, td { padding: 12px 20px; text-align: center; border: 1px solid #ddd; }
    th { background-color: #f4f4f4; color: #333; }
    td { background-color: #f9f9f9; }
    input[type="number"] { width: 80px; padding: 6px; font-size: 14px; text-align: center; border-radius: 4px; border: 1px solid #ddd; }
    button { padding: 8px 16px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; }
    button:hover { background-color: #45a049; }
    .btn-container { text-align: center; margin-top: 20px; }
    .footer { text-align: center; font-size: 12px; color: #888; margin-top: 40px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>PID Ayarları - Servo Kontrol</h1>
    <table>
      <tr>
        <th>Servo</th>
        <th>Kp</th>
        <th>Ki</th>
        <th>Kd</th>
        <th>Güncelle</th>
      </tr>
      <!-- Servo PID değerleri -->
    </table>

   
  </div>

  <div class="footer">
    <p>© 2025 Gevoid Yazılım</p>
  </div>

  <script>
    async function loadPIDValues() {
      try {
        const response = await fetch("/getPID");
        const data = await response.json();

        // PID değerlerini tabloya yerleştir
        let tableContent = '';
        for (let i = 0; i < 6; i++) {
          tableContent += `
            <tr>
              <td>${i+1}</td>
              <td><input type="number" id="kp${i}" step="0.01" value="${data[i].kp}"></td>
              <td><input type="number" id="ki${i}" step="0.01" value="${data[i].ki}"></td>
              <td><input type="number" id="kd${i}" step="0.0001" value="${data[i].kd}"></td>
              <td><button onclick="sendPID(${i})">Gönder</button></td>
            </tr>
          `;
        }

        // Tablonun içeriğini güncelle
        document.querySelector('table').innerHTML += tableContent;
      } catch (error) {
        alert("PID verileri alınamadı: " + error);
      }
    }

    async function sendPID(index) {
      const kp = document.getElementById(`kp${index}`).value;
      const ki = document.getElementById(`ki${index}`).value;
      const kd = document.getElementById(`kd${index}`).value;

      try {
        const response = await fetch("/setPID", {
          method: "POST",
          headers: {
            "Content-Type": "application/json"
          },
          body: JSON.stringify({
            servo: index,
            kp: parseFloat(kp),
            ki: parseFloat(ki),
            kd: parseFloat(kd)
          })
        });

        const data = await response.text();
        alert(data); // Başarı veya hata mesajını göster
      } catch (err) {
        alert("Hata: " + err);
      }
    }

    // Sayfa yüklendiğinde PID değerlerini yükle
    window.onload = loadPIDValues;
  </script>
</body>
</html>
)rawliteral";

String incomingData = "";

void registerControlRoutes() {

  // HTTP yolları ve fonksiyonları tanımlanıyor

  // bilgi yolu
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "6 Eklemli Robot Kol Kontrol Web Servisi.");
  });

  //bağlantıyı test etmek için yol
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  //sıcaklık verisini okumak için yol
  server.on("/temp", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(tempValue));
  });

  //uzaklık verisini okumak için yol
  server.on("/distance", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(distanceValue));
  });

  //servoların mevcut açılarını okumak için yol
  server.on("/getCurrentServoAngles", HTTP_GET, [](AsyncWebServerRequest *request) {
    String result = "";

    for (int i = 0; i < 6; i++) {
      result += String(servos[i].readMicroseconds());
      if (i < 5) {
        result += ",";
      }
    }

    request->send(200, "text/plain", result);
  });

  //servoları istenilen açıya getirmek için kullanılan yol
  server.on("/servo", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    String number;
    String angle;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam("number") && request->hasParam("angle")) {
      number = request->getParam("number")->value();
      angle = request->getParam("angle")->value();
      inputMessage = number + " -> " + angle;

      moveServoSmooth(number.toInt(), angle.toInt());

    } else {
      inputMessage = "No message sent";
    }
    //Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  //servoları adım adım hareket ettirmek için kullanılan yol
  server.on("/servoStep", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("number") && request->hasParam("desired")) {
      int number = request->getParam("number")->value().toInt();
      String desired = request->getParam("desired")->value();

      if (number >= 1 && number <= 6) {
        int index = number - 1;

        if (desired == "increase") {
          desiredDirection[index] = 1;
        } else if (desired == "decrease") {
          desiredDirection[index] = -1;
        }

        lastPressTime[index] = millis();
      }
    }

    request->send(200, "text/plain", "OK");
  });

  //fonksiyonu başlatmak için kullanılan yol
  server.on(
    "/runMoveFunction", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    nullptr,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // Veriyi biriktir
      for (size_t i = 0; i < len; i++) {
        incomingData += (char)data[i];
      }

      // Tüm veri geldiyse
      if (index + len == total) {
        Serial.println("Tüm veri geldi:");
        Serial.println(incomingData);

        if (isMoving) {
          request->send(409, "text/plain", "Zaten bir hareket işlemi çalışıyor.");
          incomingData = "";
          return;
        }

        if (parseAndMove(incomingData)) {
          request->send(200, "text/plain", "Hareket başladı.");
        } else {
          request->send(400, "text/plain", "Geçersiz JSON veya hareket verisi.");
        }

        incomingData = "";  // Yeni istekler için sıfırla
      }
    });


  
  //fonksiyon durumunu okuyabileceğimiz bir yol
  server.on("/moveFunctionStatus", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (isMoving) {
      request->send(200, "text/plain", "RUNNING");
    } else {
      request->send(200, "text/plain", "DONE");
    }
  });

  // acil stop yolu
  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request) {
    isMoving = false;  // Hareket durduruluyor
    currentCommandIndex = 0;
    resetServoPID();  // PID kontrolünü sıfırlayın
    request->send(200, "text/plain", String("OK"));
  });

  //pid değişkeni setleme yolu
  server.on(
    "/setPID", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      String body = String((char *)data).substring(0, len);

      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, body);

      if (error) {
        request->send(400, "text/plain", "Invalid JSON format");
        return;
      }

      int servo = doc["servo"];
      float kp = doc["kp"];
      float ki = doc["ki"];
      float kd = doc["kd"];

      if (servo >= 0 && servo < 6) {
        servoPID[servo].kp = kp;
        servoPID[servo].ki = ki;
        servoPID[servo].kd = kd;
        request->send(200, "text/plain", "PID Güncellendi.");
      } else {
        request->send(400, "text/plain", "Geçersiz servo numarası.");
      }
    });

  //pid değişkeni okuma yolu
  server.on("/getPID", HTTP_GET, [](AsyncWebServerRequest *request) {
    String response = "[";

    for (int i = 0; i < 6; i++) {
      response += "{\"kp\": " + String(servoPID[i].kp) + ", \"ki\": " + String(servoPID[i].ki) + ", \"kd\": " + String(servoPID[i].kd, 6) + "}";
      if (i < 5) response += ",";
    }

    response += "]";

    request->send(200, "application/json", response);  // Yanıtı gönder
  });

  //pid düzenleme sayfasına erişim yolu
  server.on("/pidControl", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", pidControlHTML);
  });
}
