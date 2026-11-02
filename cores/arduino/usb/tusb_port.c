#include <stdint.h>
#include "../Arduino.h"

uint32_t tusb_time_millis_api(void) {
    return (uint32_t) millis();
}
