#include "cache.h"

#include <stm32h725xx.h>

namespace Ion {
namespace Device {
namespace Cache {

void enable() {
  enableICache();
  enableDCache();
}

void disable() {
  disableDCache();
  disableICache();
}

void invalidateDCache() {
  SCB_InvalidateDCache();
}

void cleanDCache() {
  SCB_CleanDCache();
}

void enableDCache() {
  SCB_EnableDCache();
}

void disableDCache() {
  SCB_DisableDCache();
}

void invalidateICache() {
  SCB_InvalidateICache();
}

void enableICache() {
  SCB_EnableICache();
}

void disableICache() {
  SCB_DisableICache();
}

}
}
}
