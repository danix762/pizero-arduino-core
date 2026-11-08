#include <BCM2835Extras.h>

// Wire GPIO 4 (header pin 7) to GPIO 17 (header pin 11) with a jumper.
volatile uint32_t edges = 0;
void onEdge() { edges++; }

void setup() {
  Serial.begin(115200);
  delay(1500);
  pinMode(17, INPUT_PULLDOWN);
  attachInterrupt(17, onEdge, RISING);
  uint32_t actual = BCM.gpclkStart(4, 5000);          // GPCLK0 on GPIO 4
  Serial.print("requested 5000 Hz, actual: "); Serial.println((unsigned long)actual);
}

void loop() {
  noInterrupts(); edges = 0; interrupts();
  delay(1000);
  noInterrupts(); uint32_t n = edges; interrupts();
  Serial.print("rising edges in 1 s (expect about 5000): "); Serial.println((unsigned long)n);
}
