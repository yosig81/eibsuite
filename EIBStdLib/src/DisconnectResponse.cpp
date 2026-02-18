#include "DisconnectResponse.h"

using namespace EibStack;

CDisconnectResponse::CDisconnectResponse(unsigned char* data):
CEIBNetPacket<EIBNETIP_DISCONNECT_RESPONSE>(data)
{
	_data.channelid = data[0];
    _data.status = data[1];
}

CDisconnectResponse::CDisconnectResponse(unsigned char channelid, unsigned char status) :
CEIBNetPacket<EIBNETIP_DISCONNECT_RESPONSE>(DISCONNECT_RESPONSE)
{
	_data.channelid = channelid;
    _data.status = status;
}

CDisconnectResponse::~CDisconnectResponse()
{
}

CString CDisconnectResponse::GetStatusString() const
{
	switch (_data.status)
	{
		case (E_CONNECTION_ID):
			return ("Wrong channel id");
		case (E_NO_ERROR):
			return ("Connection OK");
		default:
			return ("unknown status");
	}

	return EMPTY_STRING;
}

void CDisconnectResponse::FillBuffer(unsigned char* buffer, int max_length)
{
	CEIBNetPacket<EIBNETIP_DISCONNECT_RESPONSE>::FillBuffer(buffer,max_length);
	unsigned char* tmp_ptr = buffer + GetHeaderSize();
	memcpy(tmp_ptr,&_data,GetDataSize());
}
