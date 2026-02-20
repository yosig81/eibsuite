#include "Emulator-ng.h"

using namespace std;

void usage()
{
	cout << "Emulator-ng Version: " << "1.0.0" << " GPLV2" << endl;
	cout << "Usage: Emulator-ng [OPTION]" << endl;
	cout << "Available options:" << endl;
	cout << '\t' << "-i Interactive mode for creating the configuration file" << endl;
	cout << '\t' << "-h prints this message and exit" << endl << endl;
	cout << "Report Emulator-ng bugs to yosig81@gmail.com" << endl << endl;
}

void emulator_main(bool interactive_conf)
{
	if(interactive_conf){
		CEIBEmulator::GetInstance().InteractiveConf();
		exit(0);
	}
	
	bool initialized = CEIBEmulator::GetInstance().Init();
	if(initialized){
		CEIBEmulator::GetInstance().Run(NULL);
	}
	else{
		cerr << "Error initializating EIB Emulator." << endl;
	}

	CEmulatorCmd::StartLoop();
	CEIBEmulator::GetInstance().Close();
	CEIBEmulator::GetInstance().Destroy();
}

int main(int argc, char **argv)
{
	bool interactive_conf = false;

	int c;
	opterr = 0;

	while ((c = getopt (argc, argv, "ih :")) != -1)
	{
		switch(c)
		{
		case 'i': interactive_conf = true;
			break;
		case 'h':
			usage();
			exit(0);
			break;
		default:
			usage();
			exit(1);
			break;
		}
	}

	emulator_main(interactive_conf);
	return 0;
}
