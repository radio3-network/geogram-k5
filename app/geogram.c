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

#define SHORT_BEEP_MAX 75
#define GAP_THRESHOLD 130
#define RESET_THRESHOLD 400
#define TRIGGER_SEQUENCE "-.-.-."

void GEOGRAM_Hook(void) {
    gGeogramTime++;

    uint16_t micLevel = BK4819_ReadRegister(0x64) & 0x7FFF;
    bool isSounding = (micLevel > 100);

    static bool inBeep = false;
    static uint32_t beepStartTime = 0;
    static uint32_t lastBeepEndTime = 0;
    static bool justUpdated = false;

    static char morseSequence[12] = "";
    static uint8_t seqPos = 0;
    static char lastDisplay[22] = "Waiting...";
    static bool isUnlocked = false;
    static bool ignoreNextSequence = true;

    if (isSounding) {
        if (!inBeep) {
            inBeep = true;
            beepStartTime = gGeogramTime;

            if ((gGeogramTime - lastBeepEndTime) > RESET_THRESHOLD) {
                isUnlocked = false;
                ignoreNextSequence = true;
            }
        }
        lastBeepEndTime = gGeogramTime;
        justUpdated = false;
    } else {
        if (inBeep) {
            inBeep = false;
            uint32_t duration = gGeogramTime - beepStartTime;

            if (seqPos < sizeof(morseSequence) - 1) {
                morseSequence[seqPos++] = (duration < SHORT_BEEP_MAX) ? '.' : '-';
                morseSequence[seqPos] = '\0';
            }
        }

        if (!justUpdated &&
            (gGeogramTime - lastBeepEndTime > GAP_THRESHOLD) &&
            (seqPos > 0)) {

            if (!isUnlocked) {
                if (strcmp(morseSequence, TRIGGER_SEQUENCE) == 0) {
                    isUnlocked = true;
                    ignoreNextSequence = false;  // ✅ Fix: don't ignore after trigger
                    strcpy(lastDisplay, "[START]");
                } else {
                    strcpy(lastDisplay, "Waiting...");
                }
            } else {
                if (ignoreNextSequence) {
                    strcpy(lastDisplay, "Ignoring...");
                    ignoreNextSequence = false;
                } else {
                    strcpy(lastDisplay, morseSequence);
                }
            }

            morseSequence[0] = '\0';
            seqPos = 0;
            justUpdated = true;
        }
    }

    // Display
    UI_DisplayClear();
    sprintf(String, "Last: %s", lastDisplay);
    UI_PrintStringSmallNormal(String, 0, 127, 4);

    sprintf(String, "Mic: %u", micLevel);
    UI_PrintStringSmallNormal(String, 0, 127, 6);
}
