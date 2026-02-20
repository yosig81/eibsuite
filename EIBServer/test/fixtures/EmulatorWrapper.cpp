// EmulatorWrapper.cpp -- Emulator-ng macro isolation unit.
//
// This is the ONLY translation unit that includes Emulator-ng.h.
// The emulator's LOG / DEFAULT_LOG_FILE_NAME macros are confined here
// and never conflict with the server's versions in other .cpp files.

#include "Emulator-ng.h"

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
