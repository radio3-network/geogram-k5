

    #include "driver/bk4819.h"
    #include "ui/helper.h"
    #include <string.h>
    #include "ui/inputbox.h"
    #include "ui/main.h"
    #include "ui/ui.h"
    #include "external/printf/printf.h"
    #include "driver/st7565.h"
    
    static uint32_t gGeogramTime = 0;
    static uint32_t maxTimeCountBeforeReset = 864000000; // 100 days

    char String[22];
    
    void GEOGRAM_Hook(void) {
        gGeogramTime++;
    
        // Reset time if needed (100-day wraparound guard)
        if (gGeogramTime > maxTimeCountBeforeReset)
            gGeogramTime = 0;
    
        // Read mic input level
        uint16_t micLevel = BK4819_ReadRegister(0x64) & 0x7FFF;
        bool isSounding = (micLevel > 100);  // Adjustable threshold
    
        static bool inBeep = false;
        static uint32_t lastBeepEndTime = 0;
        static uint8_t beepCounter = 0;
        static uint8_t lastReported = 0;
    
        if (isSounding) {
            if (!inBeep) {
                inBeep = true;
                //beepStartTime = gGeogramTime;
                beepCounter++;
            }
            lastBeepEndTime = gGeogramTime;
        } else {
            if (inBeep) {
                inBeep = false;
            }
    
            // If silence has lasted more than 2 seconds (200 x 10ms)
            if ((gGeogramTime - lastBeepEndTime) > 200 && beepCounter > 0) {
                lastReported = beepCounter;
                beepCounter = 0;
            }
        }
    
        // UI display
        UI_DisplayClear();
        UI_PrintString("GEOGRAM", 0, 127, 0, 10);
    
        if (beepCounter > 0) {
            sprintf(String, "Beeped: %02u", beepCounter);
        } else {
            sprintf(String, "Last beeps: %02u", lastReported);
        }
        UI_PrintStringSmallNormal(String, 0, 127, 4);
    }
    
    
    
    
