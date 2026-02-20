#ifndef __COMMAND_SCHEDULER_HEADER__
#define __COMMAND_SCHEDULER_HEADER__

#include "JTC.h"
#include "CTime.h"
#include "EibNetwork.h"
#include "StatsDB.h"
#include "CemiFrame.h"
#include <queue>
#include <vector>
#include <string.h>

using namespace std;

typedef struct ScheduledCommand
{
	ScheduledCommand(const CTime& time, const CEibAddress& dst, unsigned char* value, unsigned char val_len)
	{
		_time = time;
		_dst = dst;
		_value_len = val_len;
		memcpy(_value,value,val_len);
	};

	CTime _time;
	CEibAddress _dst;
	unsigned char _value_len;
	unsigned char _value[MAX_EIB_VALUE_LEN];
}ScheduledCommand;

class ScheduledCommandComparison
{
public:
	ScheduledCommandComparison() {};
	virtual ~ScheduledCommandComparison() {};

	bool operator() (const ScheduledCommand& lhs, const ScheduledCommand& rhs) const
	{
		return (lhs._time > rhs._time);
	}
};

typedef priority_queue<ScheduledCommand,vector<ScheduledCommand>,ScheduledCommandComparison > SchedQueue;

class CCommandScheduler : public JTCThread, public JTCMonitor
{
public:
	CCommandScheduler();
	virtual ~CCommandScheduler();

	virtual void run();
	void Close();

	void CheckForScheduledCommand();
	bool AddScheduledCommand(const CTime& time,
							 const CEibAddress& dst,
							 unsigned char* value,
							 unsigned char val_len,
							 CString& err_str);

private:
	void SendToEIB(const CEibAddress& dst, unsigned char* value, unsigned char value_len);

	bool _stop;
	SchedQueue _schedule;
};

#endif
