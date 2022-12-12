#include <stdint.h>

typedef struct
{
  uint8_t fps;
  uint64_t lastTriggerTime;
} SkipConfig;

uint8_t checkSkip(SkipConfig *skipConfig);

uint64_t millis();

void setCurrentTime(uint64_t time);