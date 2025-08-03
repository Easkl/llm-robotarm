int ledPin = 2;

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  if (Serial.available() > 0) {
    int received = Serial.read();  // 1바이트 수신

    // ASCII 숫자('0'~'9') → 실제 숫자(0~9)로 변환
    if (received >= '0' && received <= '9') {
      int blinkCount = received - '0';

      for (int i = 0; i < blinkCount; i++) {
        digitalWrite(ledPin, HIGH);
        delay(300);
        digitalWrite(ledPin, LOW);
        delay(300);
      }
    }
  }
}
