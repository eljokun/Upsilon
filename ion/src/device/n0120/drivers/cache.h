#ifndef ION_DEVICE_N0120_CACHE_H
#define ION_DEVICE_N0120_CACHE_H

namespace Ion {
namespace Device {
namespace Cache {

inline void dmb() {
  __asm volatile("dmb 0xF" ::: "memory");
}

inline void dsb() {
  __asm volatile("dsb 0xF" ::: "memory");
}

inline void isb() {
  __asm volatile("isb 0xF" ::: "memory");
}

void enable();
void disable();

void invalidateDCache();
void cleanDCache();
void enableDCache();
void disableDCache();

void invalidateICache();
void enableICache();
void disableICache();

}
}
}

#endif
