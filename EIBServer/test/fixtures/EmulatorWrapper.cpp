// EmulatorWrapper.cpp -- Emulator-ng macro isolation unit.
//
// This is the ONLY translation unit that includes Emulator-ng.h.
// The emulator's LOG / DEFAULT_LOG_FILE_NAME macros are confined here
// and never conflict with the server's versions in other .cpp files.

#include "Emulator-ng.h"
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>

bool InitEmulator()
{
    return CEIBEmulator::GetInstance().Init();
}

void StartEmulator()
{
    CEIBEmulator::GetInstance().Run(NULL);
}

void StopEmulator()
{
    CEIBEmulator::GetInstance().Close();
}

void SuppressEmulatorScreen()
{
    CEIBEmulator::GetInstance().GetLog().SetPrompt(false);
}

void EmulatorSendIndication(const char* group_address,
                            const unsigned char* value,
                            int value_len)
{
    CGroupEntry ge;
    ge.SetAddress(CEibAddress(group_address));
    ge.SetPhyAddress(CEibAddress("1.0.1"));
    ge.SetValue((char*)value, value_len);
    ge.SetValueLen(value_len);
    CEIBEmulator::GetInstance().GetHandler().SendIndication(ge);
}

void EmulatorGenerateRandomIndications(int count, int delay_ms)
{
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = true;
    }

    CEmulatorHandler& handler = CEIBEmulator::GetInstance().GetHandler();
    CEibAddress phy_addr((unsigned int)0xFFFF, false);

    for (int i = 0; i < count; ++i) {
        unsigned char main_grp = rand() % 16;
        unsigned char mid_grp = rand() % 8;
        unsigned char sub_grp = rand() % 256;
        unsigned short raw = ((unsigned short)main_grp << 11) |
                             ((unsigned short)mid_grp << 8) |
                             (unsigned short)sub_grp;
        CEibAddress group_addr(raw, true);

        int val_len = 1 + (rand() % 4);
        char val[14];
        for (int j = 0; j < val_len; ++j) {
            val[j] = (char)(rand() % 256);
        }

        CGroupEntry ge;
        ge.SetAddress(group_addr);
        ge.SetPhyAddress(phy_addr);
        ge.SetValue(val, val_len);
        ge.SetValueLen(val_len);

        handler.SendIndication(ge);

        if (delay_ms > 0 && i < count - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    }
}
