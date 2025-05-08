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

typedef struct {
    const char *morse;
    char letter;
} MorseMap;

static const MorseMap morseTable[] = {
    // Letters
    {".-", 'A'},   {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'},
    {".", 'E'},    {"..-.", 'F'}, {"--.", 'G'},  {"....", 'H'},
    {"..", 'I'},   {".---", 'J'}, {"-.-", 'K'},  {".-..", 'L'},
    {"--", 'M'},   {"-.", 'N'},   {"---", 'O'},  {".--.", 'P'},
    {"--.-", 'Q'}, {".-.", 'R'},  {"...", 'S'},  {"-", 'T'},
    {"..-", 'U'},  {"...-", 'V'}, {".--", 'W'},  {"-..-", 'X'},
    {"-.--", 'Y'}, {"--..", 'Z'},

    // Numbers
    {"-----", '0'}, {".----", '1'}, {"..---", '2'}, {"...--", '3'},
    {"....-", '4'}, {".....", '5'}, {"-....", '6'}, {"--...", '7'},
    {"---..", '8'}, {"----.", '9'},

    // Punctuation
    {".-.-.-", '.'}, {"--..--", ','}, {"..--..", '?'}, {"-.-.--", '!'},
    {"-..-.", '/'},  {"-.--.", '('},  {"-.--.-", ')'}, {".-...", '&'},
    {"---...", ':'}, {"-.-.-.", ';'}, {"-...-", '='},  {".-.-.", '+'},
    {"-....-", '-'}, {"..--.-", '_'}, {".-..-.", '"'}, {"...-..-", '$'},
    {".--.-.", '@'},

    // Custom space symbol
    {"--..-", ' '}
};

char decodeMorse(const char *morse) {
    for (size_t i = 0; i < sizeof(morseTable) / sizeof(MorseMap); i++) {
        if (strcmp(morse, morseTable[i].morse) == 0)
            return morseTable[i].letter;
    }
    return '?';
}

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
    static char decodedMessage[22] = "";
    static uint8_t decodedPos = 0;

    static char lastDisplay[22] = "Waiting...";
    static bool isUnlocked = false;
    static bool ignoreNextSequence = true;
    static bool messageEnded = false;

    if (isSounding) {
        if (!inBeep) {
            inBeep = true;
            beepStartTime = gGeogramTime;

            // Long silence => end of sentence
            if ((gGeogramTime - lastBeepEndTime) > RESET_THRESHOLD) {
                isUnlocked = false;
                ignoreNextSequence = true;
                messageEnded = true;
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
                    ignoreNextSequence = false;
                    strcpy(lastDisplay, "[START]");
                    decodedPos = 0;
                    decodedMessage[0] = '\0';
                } else {
                    strcpy(lastDisplay, "Waiting...");
                }
            } else {
                if (ignoreNextSequence) {
                    strcpy(lastDisplay, "Ignoring...");
                    ignoreNextSequence = false;
                } else {
                    char decoded = decodeMorse(morseSequence);
                    if (decodedPos < sizeof(decodedMessage) - 1) {
                        decodedMessage[decodedPos++] = decoded;
                        decodedMessage[decodedPos] = '\0';
                    }
                }
            }

            morseSequence[0] = '\0';
            seqPos = 0;
            justUpdated = true;
        }

        // After a full RESET_THRESHOLD silence — show final decoded message
        if (messageEnded && (gGeogramTime - lastBeepEndTime > RESET_THRESHOLD)) {
            if (decodedPos > 0) {
                decodedMessage[decodedPos] = '\0';
                strncpy(lastDisplay, decodedMessage, sizeof(lastDisplay));
            }
            decodedMessage[0] = '\0';
            decodedPos = 0;
            messageEnded = false;
        }
    }

    // Display
    UI_DisplayClear();
    sprintf(String, "Last: %s", lastDisplay);
    UI_PrintStringSmallNormal(String, 0, 127, 4);

    sprintf(String, "Mic: %u", micLevel);
    UI_PrintStringSmallNormal(String, 0, 127, 6);
}
