#include "LogFile.h"
#include "ConsoleColor.h"

CLogFile::CLogFile():
_print2screen(true),
_log_level(LOG_LEVEL_INFO),
_print_meth(NULL)
{
}

CLogFile::~CLogFile()
{
}

void CLogFile::SetConsoleColor(TEXT_COLOR color)
{
#ifdef WIN32
	switch (color)
	{
	case WHITE: cout << white;
		break;
	case BLUE: cout << blue;
		break;
	case RED: cout << red;
		break;
	case GREEN: cout << green;
		break;
	case YELLOW: cout << yellow;
		break;
	}
#else
	switch (color)
	{
		case WHITE: cout << "\033[00;0m";
			break;
		case BLUE: cout << "\033[22;34m";
			break;
		case RED: cout << "\033[22;31m";
			break;
		case GREEN: cout << "\033[22;32m";
			break;
		case YELLOW: cout << "\033[01;33m";
			break;
	}
#endif
}

void CLogFile::Init(const CString& file_name)
{
	JTCSynchronized sync(*this);
	_file_name = file_name;

	if (_file_name.IsEmpty())
	{
		return;
	}

	RotateLogIfNeeded();

	_file.clear();
	_file.open(_file_name.GetBuffer(), ios::out | ios::app);
	if (_file.fail())
	{
		_file.clear();
		throw CEIBException(FileError, "Cannot initialize log file: %s", _file_name.GetBuffer());
	}
	_file.close();
}

void CLogFile::AppendTimeLine()
{
	_file << '[';
	CTime t;
	_file << t.Format().GetBuffer();
	_file << ']';
}

void CLogFile::RotateLogIfNeeded()
{
	if (!HasFileTarget())
	{
		return;
	}

	int size = CUtils::GetFileSize(_file_name);
	if (size < 0 || size <= LOG_FILE_MAX_SIZE)
	{
		return;
	}

	CString stamp;
	CUtils::GetTimeStrForFile(stamp);
	CString rotated = _file_name + "." + stamp + ".old";
	if (rename(_file_name.GetBuffer(), rotated.GetBuffer()) != 0)
	{
		throw CEIBException(FileError, "Error rotating log file: %s", _file_name.GetBuffer());
	}
}

bool CLogFile::HasScreenTarget() const
{
	return _print2screen && _print_meth != NULL;
}

bool CLogFile::HasFileTarget() const
{
	return !_file_name.IsEmpty();
}

const char* CLogFile::LevelPrefix(LogLevel level) const
{
	switch (level)
	{
	case LOG_LEVEL_ERROR:
		return LOG_LEVEL_ERROR_STR;
	case LOG_LEVEL_DEBUG:
		return LOG_LEVEL_DEBUG_STR;
	case LOG_LEVEL_INFO:
	default:
		return LOG_LEVEL_INFO_STR;
	}
}

void CLogFile::Log(LogLevel level, const char* format,...)
{
	if (_log_level < level || format == NULL)
	{
		return;
	}

	if (!HasScreenTarget() && !HasFileTarget())
	{
		return;
	}

	char status[1024];
	status[0] = '\0';

	va_list arglist;
	va_start(arglist, format);
	vsnprintf(status, sizeof(status), format, arglist);
	va_end(arglist);
	status[sizeof(status) - 1] = '\0';

	JTCSynchronized sync(*this);

	if (HasFileTarget())
	{
		RotateLogIfNeeded();
		_file.clear();
		_file.open(_file_name.GetBuffer(), ios::out | ios::app);
		if (_file.fail())
		{
			_file.clear();
			throw CEIBException(FileError, "Cannot open log file: %s", _file_name.GetBuffer());
		}

		AppendTimeLine();
		_file << ' ' << LevelPrefix(level) << ' ' << status << endl;
		_file.close();
	}

	if (HasScreenTarget())
	{
		const bool use_console_color = (_print_meth == printf);
		if (level == LOG_LEVEL_ERROR && use_console_color)
		{
			SetConsoleColor(RED);
		}

		_print_meth("%s", status);
		_print_meth("\n");

		if (level == LOG_LEVEL_ERROR && use_console_color)
		{
			SetConsoleColor(WHITE);
		}
	}
}

void CLogFile::Log(LogLevel level, const CString& data)
{
	Log(level, "%s", data.GetBuffer());
}
