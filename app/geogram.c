#include "driver/bk4819.h"
#include "ui/helper.h"
#include <string.h>
#include "ui/inputbox.h"
#include "ui/main.h"
#include "ui/ui.h"
#include "external/printf/printf.h"
#include "driver/st7565.h"

static uint32_t gGeogramTime = 0;
char String[22];

// Morse detection thresholds (in counts)
#define SHORT_BEEP_MAX 75   // <75 counts = dot (.)
#define LONG_BEEP_MIN 75    // ≥75 counts = dash (-)
#define GAP_THRESHOLD 130   // Gap between characters

void GEOGRAM_Hook(void) {
    gGeogramTime++;

    // Read mic input
    uint16_t micLevel = BK4819_ReadRegister(0x64) & 0x7FFF;
    bool isSounding = (micLevel > 100);

    static bool inBeep = false;
    static uint32_t beepStartTime = 0;
    static uint32_t lastBeepEndTime = 0;
    static char morseSequence[12] = "";  // Stores current pattern (e.g. ".-.")
    static uint8_t seqPos = 0;
    static char lastDisplay[22] = "No beeps";
    static bool justUpdated = false;

    if (isSounding) {
        if (!inBeep) {
            inBeep = true;
            beepStartTime = gGeogramTime;
        }
        lastBeepEndTime = gGeogramTime;
        justUpdated = false;  // Reset update flag during sound
    } else {
        if (inBeep) {
            inBeep = false;
            uint32_t duration = gGeogramTime - beepStartTime;

            if (seqPos < sizeof(morseSequence) - 1) {
                morseSequence[seqPos++] = (duration < SHORT_BEEP_MAX) ? '.' : '-';
                morseSequence[seqPos] = '\0';
            }
        }

        // Silence gap detection — do this every tick while quiet
        if (!justUpdated &&
            (gGeogramTime - lastBeepEndTime > GAP_THRESHOLD) &&
            (seqPos > 0)) {

            strcpy(lastDisplay, morseSequence);
            morseSequence[0] = '\0';
            seqPos = 0;
            justUpdated = true;  // Prevent re-updating every frame
        }
    }

    // Display
    UI_DisplayClear();

    sprintf(String, "Last: %s", lastDisplay);
    UI_PrintStringSmallNormal(String, 0, 127, 4);

    sprintf(String, "Mic: %u", micLevel);
    UI_PrintStringSmallNormal(String, 0, 127, 6);
}
