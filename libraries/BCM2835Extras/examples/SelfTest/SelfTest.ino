#include <BCM2835Extras.h>

// Set to 1 to finish by letting the watchdog reboot the board on purpose.
#define TEST_WATCHDOG_RESET 0

static int passed = 0, failed = 0;
static void check(const char *name, bool ok) {
  Serial.print(ok ? "[PASS] " : "[FAIL] ");
  Serial.println(name);
  if (ok) passed++; else failed++;
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.print("sketch started, millis = "); Serial.println((unsigned long)millis());   // restarts after a reset
  Serial.println("=== BCM2835Extras self-test (no wiring needed) ===");

  uint32_t x;
  check("hardware RNG answers", BCM.randomRead(&x));
  uint32_t first = BCM.random32(), ones = 0;
  bool allSame = true;
  for (int i = 0; i < 1000; i++) {
    uint32_t v = BCM.random32();
    ones += __builtin_popcount(v);
    if (v != first) allSame = false;
  }
  check("RNG output is not constant", !allSame);
  check("RNG bits are balanced (about 16000 ones in 32000 bits)", ones > 15500 && ones < 16500);

  check("board model reported", BCM.boardRevision() != 0);
  int32_t t = BCM.temperatureMilliC();
  check("temperature plausible (0 to 100 C)", t > 0 && t < 100000);
  check("ARM clock reported", BCM.clockHz(3) > 100000000UL);
  check("UART clock reported", BCM.clockHz(2) > 1000000UL);

  BCM.watchdogStart(2000);
  uint32_t t0 = millis();
  while (millis() - t0 < 3500) { delay(100); BCM.watchdogFeed(); }
  BCM.watchdogFeed();
  uint32_t rem = BCM.watchdogRemainingMs();
  check("watchdog stays quiet while fed for 3.5 s", rem > 1500 && rem <= 2000);
  BCM.watchdogStop();
  delay(2500);
  check("watchdog stopped (board still running 2.5 s after stop)", true);

  Serial.print("passed: "); Serial.print((long)passed);
  Serial.print("   failed: "); Serial.println((long)failed);
  Serial.println(failed == 0 ? "ALL PASS" : "SOME TESTS FAILED");

#if TEST_WATCHDOG_RESET
  Serial.println("Hanging on purpose: the board should reboot in about 2 s...");
  BCM.watchdogStart(2000);
  for (;;) { }          // no feeding: the watchdog must reset us, millis() restarts near 0
#endif
}
void loop() {}
