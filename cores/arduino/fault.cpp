#include "Arduino.h"
#include "usb/usb_core.h"

static const char *hex8(uint32_t v) {
  static char b[9];
  for (int i = 7; i >= 0; i--) { b[i] = "0123456789ABCDEF"[v & 15]; v >>= 4; }
  b[8] = 0;
  return b;
}

static void put(const char *name, uint32_t v) {
  Serial.print(name); Serial.print("=0x"); Serial.print(hex8(v)); Serial.print(" ");
}

extern "C" void fault_report(uint32_t code, uint32_t spsr, uint32_t *f) {
  static const char *names[] = {"?", "UNDEFINED INSTRUCTION", "SWI", "PREFETCH ABORT",
                                "DATA ABORT", "RESERVED VECTOR", "FIQ", "RESET"};
  static const char *regs[]  = {"r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","r11","r12"};

  uint32_t dfsr, dfar, ifsr, ifar;
  __asm__ volatile("mrc p15,0,%0,c5,c0,0" : "=r"(dfsr));
  __asm__ volatile("mrc p15,0,%0,c6,c0,0" : "=r"(dfar));
  __asm__ volatile("mrc p15,0,%0,c5,c0,1" : "=r"(ifsr));
  __asm__ volatile("mrc p15,0,%0,c6,c0,2" : "=r"(ifar));

  pinMode(47, OUTPUT);                 // ACT LED changes state at once, even if USB is dead
  digitalWrite(47, HIGH);

  for (;;) {
    Serial.print("\r\n*** ");
    Serial.print(code < 8 ? names[code] : names[0]);
    Serial.println(" ***");
    put("PC", f[13]); put("SPSR", spsr); Serial.println("");
    if (code == 4) { put("DFAR", dfar); put("DFSR", dfsr); Serial.println(""); }
    if (code == 3) { put("IFAR", ifar); put("IFSR", ifsr); Serial.println(""); }
    for (int i = 0; i < 13; i++) put(regs[i], f[i]);
    Serial.println("");

    for (int k = 0; k < 10; k++) {     // blink for a second, keeping USB serviced
      digitalWrite(47, k & 1);
      delay(100);
    }
  }
}
