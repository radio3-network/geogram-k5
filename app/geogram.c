#include "driver/bk4819.h"
#include "ui/helper.h"
#include <string.h>
#include "ui/inputbox.h"
#include "ui/main.h"
#include "ui/ui.h"
#include "external/printf/printf.h"
#include "driver/st7565.h"


void processInput(const char *input);


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
                processInput(lastDisplay);
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


// Helper: check if character is digit
int is_digit(char c) {
    return c >= '0' && c <= '9';
}

// Helper: string length
int simple_strlen(const char *s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

// BK4819_SetFrequency
void processCommand_M(const char *args) {
    if (!is_digit(args[0]) || !is_digit(args[1]) || args[2] != ':')
        return;

    int channel = (args[0] - '0') * 10 + (args[1] - '0');

    const char *freqStr = args + 3;
    char freqBuf[16];
    int i = 0;
    while (i < 15 && ((freqStr[i] >= '0' && freqStr[i] <= '9') || freqStr[i] == '.')) {
        freqBuf[i] = freqStr[i];
        i++;
    }
    freqBuf[i] = '\0';

    // Parse MHz and kHz as integers to avoid floating point
    unsigned long mhz = 0, khz = 0;
    const char *dot = freqBuf;
    while (*dot && *dot != '.') dot++;
    if (*dot == '.') {
        for (int j = 0; j < (dot - freqBuf); ++j)
            mhz = mhz * 10 + (freqBuf[j] - '0');

        for (int j = 1; j <= 5 && dot[j]; ++j)
            khz = khz * 10 + (dot[j] - '0');

        while (khz < 10000) khz *= 10; // pad to 5 digits
    }

    UI_DisplayClear();
    sprintf(String, "MEM:%d F:%lu.%05lu MHz", channel, mhz, khz);
    UI_PrintStringSmallNormal(String, 0, 127, 4);
}

void processInput(const char *input) {
    if (!input || simple_strlen(input) < 1)
        return;

    char command = input[0];
    const char *args = input + 1;

    switch (command) {
        case 'M':
            processCommand_M(args);
            break;
        // Add future command cases here
        default:
            break;
    }
}
