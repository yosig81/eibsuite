#include "EmulatorCmd.h"
#include "Utils.h"
#include "cli.h"
#include "Emulator-ng.h"
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>

CEmulatorCmd::CEmulatorCmd()
{
}

CEmulatorCmd::~CEmulatorCmd()
{
}

void CEmulatorCmd::PrintAvailableCmds()
{
	LOG_SCREEN("\n");
	LOG_SCREEN("q - Quit the emulator program\n");
	LOG_SCREEN("p - Print the current Emulator database\n");
	LOG_SCREEN("s - Send indication to specific group\n");
	LOG_SCREEN("g - Generate random indications\n");
	LOG_SCREEN("d - Disconnect any connected client\n");
}

void CEmulatorCmd::StartLoop()
{
	static CString msg("Enter command: [Press '?' for list of available commands]\n");
	static CString input;
	while(true)
	{
		CUtils::WaitForInput(input, msg, false);
		input.ToLower();
		if(input == "q"){
			break;
		}else if(input == "?"){
			PrintAvailableCmds();
		}else if(input == "p"){
			CEIBEmulator::GetInstance().GetDB().Print();
		}else if(input == "s"){
			HandleSendCommand();
		}else if(input == "g"){
			HandleGenerateCommand();
		}else if (input == "d"){
			CEIBEmulator::GetInstance().GetHandler().DisconnectClients();
		}else{
			LOG_SCREEN("Unknown command. [Press '?' for list of available commands]\n");
		}

	}
}

void CEmulatorCmd::HandleGenerateCommand()
{
	CEmulatorHandler& handler = CEIBEmulator::GetInstance().GetHandler();

	int count = 0;
	ConsoleCLI::GetIntRange("Number of indications to send: ", count, 1, 10000, 10);

	int delay_ms = 0;
	ConsoleCLI::GetIntRange("Delay between indications (ms): ", delay_ms, 0, 60000, 0);

	if (!handler.HasConnectedClients()) {
		LOG_SCREEN("No client connected. Aborting.\n");
		return;
	}

	static bool seeded = false;
	if (!seeded) {
		srand((unsigned int)time(NULL));
		seeded = true;
	}

	// Fixed physical address 15.15.255 to identify generated traffic
	CEibAddress phy_addr((unsigned int)0xFFFF, false);

	for (int i = 0; i < count; ++i) {
		// Random group address: main(0-15)/middle(0-7)/sub(0-255)
		unsigned char main_grp = rand() % 16;
		unsigned char mid_grp = rand() % 8;
		unsigned char sub_grp = rand() % 256;
		unsigned short raw = ((unsigned short)main_grp << 11) |
		                     ((unsigned short)mid_grp << 8) |
		                     (unsigned short)sub_grp;
		CEibAddress group_addr(raw, true);

		// Random value: 1-4 bytes
		int val_len = 1 + (rand() % 4);
		char val[MAX_EIB_VAL];
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

	LOG_SCREEN("Sent %d random indication(s).\n", count);
}

void CEmulatorCmd::HandleSendCommand()
{
	CEmulatorHandler& handler = CEIBEmulator::GetInstance().GetHandler();
	CEmulatorDB& db = CEIBEmulator::GetInstance().GetDB();

	int size = db.GetNumOfRecords();
	db.Print();

	CGroupEntry ge;
	int index;
	ConsoleCLI::GetIntRange("\nSelect Group: ", index, 1, size, 1);
	if(db.GetGroupEntryByIndex(index, ge)){
		handler.SendIndication(ge);
	}else{
		LOG_ERROR("\nCannot find group with index %d.", index);
	}
}
