#ifndef TPIP_HISTORY_H_
#define TPIP_HISTORY_H_

/*******************************************************************************
 * Min/max history buffer
 ******************************************************************************/

#include "../CC110x/definitions.h"
#include "../settings.h"

/*
 * How many radio broadcasts in one hour (approximately).
 * Need to keep parentheses here because this is a divisor.
 * DO NOT MODIFY THESE VALUES without adjusting the broadcast algorithms.
 */
#define RADIO_PER_HOUR (MS_ONE_HOUR/PACKTINTVL_MS)
#define MAX_HISTORY_VALUES 28

typedef struct {
	signed char minTemp;
	signed char maxTemp;
	signed char minHumid;
	signed char maxHumid;
	uint8_t minMaxLight;
} HistoryUnit;

extern uint8_t historyIndex;
extern uint8_t historyRegValue;



void initHistory();
void addHistory(const signed char temp, const signed char humidity,
		uint8_t light);
HistoryUnit* getHistoryAt(const uint32_t hoursAgo);
HistoryUnit* getHistory(void);



#endif /* TPIP_HISTORY_H_ */
