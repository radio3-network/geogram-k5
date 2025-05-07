/*
 * -----------------------------------------------------------------------------
 *  GEOGRAM — Beep-Based Transmission Protocol with Blossom Filtering
 * -----------------------------------------------------------------------------
 *
 *  This protocol uses amplitude and timing of microphone input to detect a
 *  calibration sequence followed by a stream of data beeps.
 *
 *  Tone sequence:
 *
 *      Time ──────────────────────────────────────────────────────────────▶
 *
 *      ┌────────────┐         ┌───────┐         ┌──────┐       data beeps...
 *      │   HIGH     │         │  MID  │         │ LOW  │
 *      └────────────┘         └───────┘         └──────┘
 *           ▲                     ▲                 ▲
 *        Calibrate            Calibrate          Calibrate
 *
 *   - HIGH:  >1000ms beep to trigger VOX and record HIGH average
 *   - MID:   short beep, calibrates MID average + interval
 *   - LOW:   calibrates LOW average
 *   - DATA:  beeps classified using these thresholds
 *   - END:   protocol resets after 2 seconds of silence
 * -----------------------------------------------------------------------------
 */

#include "driver/bk4819.h"
#include "ui/helper.h"
//#include <stddef.h>  // for size_t
//#include <app/flashlight.h>
#include <string.h>
#include "ui/inputbox.h"
#include "ui/main.h"
#include "ui/ui.h"
#include "external/printf/printf.h"
#include "driver/st7565.h"


  
 static uint32_t gGeogramTime = 0;
 static uint32_t maxTimeCountBeforeReset = 864000000; // 100 days

 /*
  #define MIC_THRESHOLD             600
  #define HIGH_MIN_DURATION         15   // 100ms
  #define SILENCE_TIMEOUT           300  // 2s = 200 * 10ms
  #define BLOSSOM_ALPHA             0.2f
  #define BLOSSOM_TOLERANCE_PCT     10
  
  typedef enum {
      STATE_WAIT_FOR_HIGH,
      STATE_WAIT_FOR_MID,
      STATE_WAIT_FOR_LOW,
      STATE_TRANSMISSION
  } ProtocolState;
  
  static ProtocolState state = STATE_WAIT_FOR_HIGH;
  
  //static uint32_t beepStartTime = 0; // when the beep started
  //static uint32_t beepStopTime = 0;  // when the beep as stopped
  //static uint16_t filteredMic = 0;
  
  //static uint32_t highAvg = 0, midAvg = 0, lowAvg = 0;
  //static uint8_t  highCount = 0, midCount = 0, lowCount = 0;

  */

  char String[22];
  
 
 
 
  void GEOGRAM_Hook(void) {
     gGeogramTime++;
     // make sure we don't overflow in the time counter
     if (gGeogramTime > maxTimeCountBeforeReset) {
         gGeogramTime = 0;
     }
 
     // get the volume level right now
     uint16_t micLevel = BK4819_ReadRegister(0x64) & 0x7FFF;
     // are we above the minimum sound level?
     //bool isSounding = (micLevel > MIC_THRESHOLD);
     

    UI_DisplayClear();
    UI_PrintString("GEOGRAM!", 0, 127, 0, 10);
    sprintf(String, "SND: %s", micLevel);
    UI_PrintStringSmallNormal("Test1", 0, 127, 2);
    UI_PrintStringSmallNormal("Test2", 0, 127, 4);
    UI_PrintStringSmallNormal("Test3", 0, 128, 6);
    
  
 
 }
 
 
  
