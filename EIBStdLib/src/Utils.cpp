#include "Utils.h"
#include "JTC.h"

void CUtils::ReadFile(const CString& full_name, CString& content)
{
	content = EMPTY_STRING;
	ifstream myfile;
	myfile.open(full_name.GetBuffer(),ios::in);
	if (myfile.fail()){
		return;
	}

	CString line;
	while (getline(myfile, line.GetSTDString()))
	{
		content += line;
		content += LINE_BREAK;
	}
}

void CUtils::SaveFile(const CString& full_name, const CString& content)
{
	ofstream file(full_name.GetBuffer(), ios::out | ios::binary | ios::trunc);
	if (!file.is_open()){
		throw CEIBException(FileError, "Cannot open file for write");
	}
	file.write(content.GetBuffer(), static_cast<streamsize>(content.GetLength()));
	if (!file.good()){
		throw CEIBException(FileError, "Cannot write file");
	}
}

int CUtils::GetFileSize(const CString& full_name)
{
	ifstream file(full_name.GetBuffer(), ios::binary | ios::ate);
	if (!file.is_open()){
		return 0;
	}
	return static_cast<int>(file.tellg());
}

void CUtils::GetTimeStrForFile(CString& time_str)
{
	time_str.Clear();

	time_t raw_time;
	time(&raw_time);
	char buffer[80];
	struct tm* timeinfo = localtime(&raw_time);
	strftime (buffer,80,"%b_%d_%H_%M",timeinfo);

	time_str += buffer;
}

void CUtils::GetCurrentPath(CString& path)
{
	char current_path[_MAX_PATH];
	char* result = _getcwd(current_path,_MAX_PATH);
	if(result == NULL){
		throw CEIBException(SystemError,"Cant retreive the current path");
		//error
	}
	path = current_path;
	path += PATH_DELIM;
}

int CUtils::GetNumOfNICs()
{
#ifdef WIN32
	// Get adapter list
	/*
	LANA_ENUM AdapterList;
	NCB Ncb;
	memset(&Ncb, 0, sizeof(NCB));
	Ncb.ncb_command = NCBENUM;
	Ncb.ncb_buffer = (unsigned char *)&AdapterList;
	Ncb.ncb_length = sizeof(AdapterList);
	Netbios(&Ncb);
	return AdapterList.length;
	*/
	return 0;
#else

	struct ifconf ifc;
	char buff[1024];
	struct ifreq *ifr;
	int skfd;
	ifc.ifc_len = sizeof(buff);
	ifc.ifc_buf = buff;
	if ((skfd = socket(AF_INET, SOCK_DGRAM,0)) < 0) {
		printf("new socket failed\n");
		exit(1);
	}
	if (ioctl(skfd, SIOCGIFCONF, &ifc) < 0) {
		printf("SIOCGIFCONF:Failed \n");
		exit(1);
	}
	ifr = ifc.ifc_req;
	return (ifc.ifc_len / sizeof(struct ifreq));
#endif
}

bool CUtils::EnumNics(map<CString,CString>& nics)
{
#ifdef WIN32
	// Get adapter list
	IP_ADAPTER_INFO AdapterInfo[16];       // Allocate information for up to 16 NICs
	DWORD dwBufLen = sizeof(AdapterInfo);  // Save memory size of buffer

	DWORD dwStatus = GetAdaptersInfo(
							AdapterInfo,// [out] buffer to receive data
							&dwBufLen); // [in] size of receive data buffer
  
  	//assert(dwStatus == ERROR_SUCCESS);  // Verify return value is valid, no buffer overflow

  	PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo; // Contains pointer to current adapter info
	nics.clear();
	int i = 0;
  	do
  	{
		pair<CString,CString> p;
		p.first = CString(pAdapterInfo->Index);
		p.second += pAdapterInfo->Description; //pAdapterInfo->IpAddressList.IpAddress.String;
		nics.insert(nics.end(), p);
		pAdapterInfo = pAdapterInfo->Next;    // Progress through linked list
		++i;
  	}while(pAdapterInfo);

	return true;
#else
	char          buf[1024];
	struct ifconf ifc;
	struct ifreq *ifr;
	int           sck;
	int           nInterfaces;
	int           i;

	/* Get a socket handle. */
	sck = socket(AF_INET, SOCK_DGRAM, 0);
	if(sck < 0){
		return false;
	}
	/* Query available interfaces. */
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if(ioctl(sck, SIOCGIFCONF, &ifc) < 0){
		return false;
	}
	/* Iterate through the list of interfaces. */
	nics.clear();
	char *ptr = buf;
	char *end = buf + ifc.ifc_len;
	while(ptr < end)
	{
		struct ifreq *item = (struct ifreq *)ptr;
#ifdef __APPLE__
		size_t len = IFNAMSIZ + item->ifr_addr.sa_len;
		if(len < sizeof(struct ifreq))
			len = sizeof(struct ifreq);
#else
		size_t len = sizeof(struct ifreq);
#endif
		if(item->ifr_addr.sa_family == AF_INET)
		{
			pair<CString,CString> p;
			p.first = item->ifr_name;
			p.second = item->ifr_name;
			p.second += " (";
			p.second += inet_ntoa(((struct sockaddr_in *)&item->ifr_addr)->sin_addr);
			p.second += ")";
			nics.insert(nics.end(), p);
		}
		ptr += len;
	}
	return true;
#endif
}

void CUtils::WaitForCharInput(char expected, const CString& msg, bool sleep)
{
	if(sleep){
		//delay execution a bit, so we could flush any waiting buffers to screen before printing these msgs
		JTCThread::sleep(500);
	}

	char x = (char)0 ;
	while (true)
	{
		cout << endl << msg.GetBuffer() << endl;
		cin >> x ;
		if(cin.fail() || cin.eof()){
			// stdin not available (backgrounded/detached) - wait indefinitely via sleep
			cin.clear();
			while(true){
				JTCThread::sleep(60000);
			}
		}
		if(x != expected){
			cout << "Incorrect Choice." << endl;
			cin.ignore(INT_MAX,'\n');
		}
		else{
			break;
		}
	}
}

void CUtils::WaitForInput(CString& input, const CString& msg, bool sleep)
{
	if(sleep){
		//delay execution a bit, so we could flush any waiting buffers to screen before printing these msgs
		JTCThread::sleep(500);
	}

	cout << endl << msg.GetBuffer();
	getline(cin, input.GetSTDString());
}
