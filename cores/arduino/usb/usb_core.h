#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void usb_start(void);   // power, init, connect (blocks ~4 s for now)
void usb_task(void);    // service USB; call often
#ifdef __cplusplus
}
#endif
