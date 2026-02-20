#include "CommandScheduler.h"
#include "EIBServer.h"

CCommandScheduler::CCommandScheduler() :
_stop(false),
_schedule(ScheduledCommandComparison())
{
	this->setName("CommandScheduler");
}

CCommandScheduler::~CCommandScheduler()
{
}

void CCommandScheduler::run()
{
	CTime time_count;
	time_count.SetNow();
	time_count += 60;

	while(!_stop)
	{
		if(time_count.secTo() == 0){
			CheckForScheduledCommand();
			time_count.SetNow();
			time_count += 60;
		}

		// Sleep briefly to avoid busy-waiting
		JTCSynchronized sync(*this);
		try {
			this->wait(1000);
		} catch (...) {
		}
	}
}

void CCommandScheduler::Close()
{
	_stop = true;
	JTCSynchronized sync(*this);
	this->notify();
}

void CCommandScheduler::CheckForScheduledCommand()
{
	JTCSynchronized sync(*this);

	if(_schedule.empty()){
		return;
	}

	ScheduledCommand cmd = _schedule.top();
	CTime t;
	t.SetNow();

	while(cmd._time <= t)
	{
		_schedule.pop();
		SendToEIB(cmd._dst, cmd._value, cmd._value_len);
		if(_schedule.empty()){
			return;
		}
		cmd = _schedule.top();
	}
}

void CCommandScheduler::SendToEIB(const CEibAddress& dst, unsigned char* value, unsigned char value_len)
{
	START_TRY
		CEIBInterface& iface = CEIBServer::GetInstance().GetEIBInterface();

		CCemi_L_Data_Frame msg;
		msg.SetMessageControl(L_DATA_REQ);
		msg.SetAddilLength(0);
		msg.SetCtrl1(0);
		msg.SetCtrl2(6);
		msg.SetPriority(PRIORITY_NORMAL);
		msg.SetFrameFormatStandard();
		msg.SetSrcAddress(CEibAddress());
		msg.SetDestAddress(dst);
		msg.SetValue(value, value_len);

		iface.GetOutputHandler()->Write(msg, NON_BLOCKING, NULL);
		LOG_DEBUG("[Scheduler] Sent scheduled command to %s", dst.ToString().GetBuffer());
	END_TRY_START_CATCH(e)
		LOG_ERROR("[Scheduler] Failed to send scheduled command: %s", e.what());
	END_CATCH
}

bool CCommandScheduler::AddScheduledCommand(const CTime& time,
										const CEibAddress& dst,
										unsigned char* value,
										unsigned char val_len,
										CString& err_str)
{
	JTCSynchronized sync(*this);

	CTime t;
	t.SetNow();
	if(time <= t){
		err_str += "Cannot schedule task in the past. a Time Machine should be used instead.";
		return false;
	}

	_schedule.push(ScheduledCommand(time,dst,value,val_len));

	return true;
}
