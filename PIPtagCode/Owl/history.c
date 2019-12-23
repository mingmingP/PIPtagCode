#include "history.h"

/* History ring buffer of values */
HistoryUnit history[MAX_HISTORY_VALUES];
/* Index for "head" of history ring buffer */
char historyHead = MAX_HISTORY_VALUES-1;

void initHistory() {
	int i = 0;
	for (; i < MAX_HISTORY_VALUES; ++i) {
		HistoryUnit* foo = &history[i];
		foo->minTemp = 127;
		foo->maxTemp = -127;
		foo->minHumid = 127;
		foo->maxHumid = -127;
		/* Maximum value is in high nibble. */
		foo->minMaxLight = 0x0F;
	}
}

/* Number of radio transmissions since power-on */
extern uint32_t numRadio;

/*
 * The "hour" of the head of the history buffer.
 * Based on how many transmissions made since power-on.
 */
uint32_t historyHeadHour = 0;

/**
 * Current index into history sequence.
 */
uint8_t historyIndex = 0;
/**
 * The "normal" (non-last) value sent in the history sequence.
 */
uint8_t historyRegValue = 1;



/*
 * Temperature and humidity are 8-bit signed values (-128 -- 127).
 * Light is an unsigned 8-bit value, but only high nibble is used.
 */
void addHistory(const signed char temp, const signed char humidity,
		const uint8_t light) {
	/* The current hour since power-on. */
	uint32_t hourNow = numRadio / RADIO_PER_HOUR;

	/* Need to "cycle" the history by ejecting oldest, inserting new. */
	if (historyHeadHour != hourNow) {
		historyHeadHour = hourNow;
		historyHead--;
		/* Reset the head pointer to the front of the buffer */
		if (historyHead >= MAX_HISTORY_VALUES) {
			historyHead = MAX_HISTORY_VALUES-1;
		}


		history[historyHead].maxTemp = temp;
		history[historyHead].minTemp = temp;
		history[historyHead].maxHumid = humidity;
		history[historyHead].minHumid = humidity;
		history[historyHead].minMaxLight = (light & 0xf0)
				| ((light >> 4) & 0x0f);
	}
	/* Update the min/max for this hour */
	else {
//		HistoryUnit* nowUnit = &history[historyHead];
		if (temp > history[historyHead].maxTemp) {
			history[historyHead].maxTemp = temp;
		}
		if (temp < history[historyHead].minTemp) {
			history[historyHead].minTemp = temp;
		}
		if (humidity > history[historyHead].maxHumid) {
			history[historyHead].maxHumid = humidity;
		}
		if (humidity < history[historyHead].minHumid) {
			history[historyHead].minHumid = humidity;
		}
		/*
		 * Max light is stored in high nibble.
		 * New light values may have all bits set (based on precision).
		 * Ignore low nibble of min/max value.
		 */
		if (light > (history[historyHead].minMaxLight & 0xf0)) {
			history[historyHead].minMaxLight = (history[historyHead].minMaxLight
					& 0x0f) | (light & 0xf0);
		}
		/*
		 * Min light is stored in low nibble.
		 * Ignore high nibble of min/max value.
		 */
		if (light < (history[historyHead].minMaxLight << 4)) {
			history[historyHead].minMaxLight = (history[historyHead].minMaxLight
					& 0xf0) | ((light >> 4) & 0x0f);
		}
	}

}

/*
 * Returns the "next" history value to broadcast. historyIndex is updated
 * within this function.  Callers who need the value of historyIndex used
 * in this function should read historyIndex BEFORE calling getHistory().
 *
 * Algorithm for "HISTORY_28" (28-hour history):
 * Every third broadcast will be the "oldest" value in memory, all other broadcasts
 * will be hours 1-27 in increasing order.
 *
 * Slot Hour
 * ---- ----
 * 0     1
 * 1     2
 * 2    27
 * 3     3
 * 4     4
 * 5    27
 * 6     5
 * ...
 */
HistoryUnit* getHistory() {
	// Default to index 0
	int retIdx = historyHead;
#ifdef HISTORY_28
	/*
	 * Broadcast "oldest" hour every 3rd transmission, else in-order of hours 1-27.
	 * Last transmission per hour is also "oldest"
	 */
	if ((historyIndex % 3 == 2) || (historyIndex == (MS_ONE_HOUR/HISTORY_INTVL)-1) ) {
		retIdx = (historyHead + MAX_HISTORY_VALUES - 1) % MAX_HISTORY_VALUES;
	}
	// Normal broadcast
	else {
		retIdx = (historyHead + historyRegValue) % MAX_HISTORY_VALUES;
		++historyRegValue;
		if(historyRegValue >= (MAX_HISTORY_VALUES-1)) {
			historyRegValue = 1;
		}
	}

#endif
	// Increment the history index pointer for the next retrieval
	++historyIndex;
#ifdef HISTORY_28
	// 40 total history broadcasts per hour
	if (historyIndex >= (MS_ONE_HOUR/HISTORY_INTVL)) {
		historyIndex = 0;
	}
#endif
	return &history[retIdx];
}

/*
 * Returns the history value
 */
HistoryUnit* getHistoryAt(const uint32_t hoursAgo) {
	return &history[(hoursAgo + historyHead) % MAX_HISTORY_VALUES];
}

