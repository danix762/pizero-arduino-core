#include <BCM2835Extras.h>

static void printHex(uint32_t v) {
  char b[11] = "0x00000000";
  for (int i = 9; i >= 2; i--) { b[i] = "0123456789ABCDEF"[v & 15]; v >>= 4; }
  Serial.print(b);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.print("board:             "); Serial.println(BCM.boardName());
  Serial.print("board revision:    "); printHex(BCM.boardRevision()); Serial.println("");
  uint64_t sn = BCM.serialNumber();
  Serial.print("serial number:     "); printHex((uint32_t)(sn >> 32)); printHex((uint32_t)sn); Serial.println("");
  Serial.print("firmware revision: "); Serial.println((unsigned long)BCM.firmwareRevision());
  Serial.print("ARM memory (MB):   "); Serial.println((unsigned long)(BCM.armMemoryBytes() / 1048576));
  Serial.print("GPU memory (MB):   "); Serial.println((unsigned long)(BCM.gpuMemoryBytes() / 1048576));
  long mc = (long)BCM.temperatureMilliC();
  Serial.print("temperature (C):   "); Serial.print(mc / 1000); Serial.print("."); Serial.println((mc % 1000) / 100);
  Serial.print("ARM clock (Hz):    "); Serial.println((unsigned long)BCM.clockHz(3));
  Serial.print("core clock (Hz):   "); Serial.println((unsigned long)BCM.clockHz(4));
  Serial.print("EMMC clock (Hz):   "); Serial.println((unsigned long)BCM.clockHz(1));
  Serial.print("UART clock (Hz):   "); Serial.println((unsigned long)BCM.clockHz(2));
  Serial.print("random numbers:    ");
  for (int i = 0; i < 4; i++) { printHex(BCM.random32()); Serial.print(" "); }
  Serial.println("");
}
void loop() {}
