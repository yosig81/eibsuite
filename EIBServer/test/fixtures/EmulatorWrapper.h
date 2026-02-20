// EmulatorWrapper.h -- Thin wrapper around Emulator-ng API.
//
// This header declares free functions that delegate to CEIBEmulator.
// It deliberately does NOT include Emulator-ng.h so that the emulator's
// LOG / DEFAULT_LOG_FILE_NAME macros never leak into translation units
// that also include EIBServer.h (which defines its own versions).
//
// Test .cpp files include this header alongside EIBServer.h -- no conflict.
// EmulatorWrapper.cpp includes Emulator-ng.h and compiles in isolation.

#ifndef EMULATOR_WRAPPER_H
#define EMULATOR_WRAPPER_H

bool InitEmulator();   // CEIBEmulator::GetInstance().Init()
void StartEmulator();  // CEIBEmulator::GetInstance().Run(NULL)
void StopEmulator();   // CEIBEmulator::GetInstance().Close()

// Send an unsolicited indication from the emulator to connected clients.
// group_address: 3-level EIB group address string (e.g. "1/2/3")
// value: byte array to send
// value_len: length of value array
void EmulatorSendIndication(const char* group_address,
                            const unsigned char* value,
                            int value_len);

#endif // EMULATOR_WRAPPER_H
