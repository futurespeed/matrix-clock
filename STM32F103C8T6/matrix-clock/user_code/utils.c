#include "utils.h"

uint64_t system_time_offset;
extern uint64_t system_run_time;

uint64_t millis()
{
	return system_run_time + system_time_offset;
}

void setCurrentTime(uint64_t time)
{
	system_time_offset = time - system_run_time;
}

uint8_t checkSkip(SkipConfig *skipConfig)
{
    uint64_t currTime = millis();
    if (currTime - skipConfig->lastTriggerTime < 1000 / skipConfig->fps)
        return 0;
    skipConfig->lastTriggerTime = currTime;
    return 1;
}