#include "LogFile.h"
#include "ConsoleColor.h"

namespace {
bool IsPathSeparator(char c)
{
	return c == '/' || c == '\\';
}

void EnsureParentDirectoryExists(const CString& file_name)
{
	const char* full_path = file_name.GetBuffer();
	if (full_path == NULL || full_path[0] == '\0')
	{
		return;
	}

	const char* sep = strrchr(full_path, '/');
	const char* back_sep = strrchr(full_path, '\\');
	if (back_sep != NULL && (sep == NULL || back_sep > sep))
	{
		sep = back_sep;
	}
	if (sep == NULL)
	{
		return;
	}

	const int dir_len = static_cast<int>(sep - full_path);
	if (dir_len <= 0)
	{
		return;
	}

	string dir(file_name.SubString(0, dir_len).GetBuffer());
	for (size_t i = 0; i < dir.size(); ++i)
	{
		if (dir[i] == '\\')
		{
			dir[i] = '/';
		}
	}

	string current;
	size_t pos = 0;
	if (!dir.empty() && IsPathSeparator(dir[0]))
	{
		current = PATH_DELIM.GetBuffer();
		pos = 1;
	}

	while (pos <= dir.size())
	{
		size_t next = dir.find('/', pos);
		const size_t len = (next == string::npos) ? (dir.size() - pos) : (next - pos);
		if (len > 0)
		{
			if (!current.empty() && current[current.size() - 1] != PATH_DELIM.GetBuffer()[0])
			{
				current += PATH_DELIM.GetBuffer();
			}
			current += dir.substr(pos, len);

			const CString current_dir(current);
			if (!CDirectory::IsExist(current_dir))
			{
				if (!CDirectory::Create(current_dir) && !CDirectory::IsExist(current_dir))
				{
					throw CEIBException(FileError, "Cannot initialize log folder: %s", current_dir.GetBuffer());
				}
			}
		}

		if (next == string::npos)
		{
			break;
		}
		pos = next + 1;
	}
}
} // namespace

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

	EnsureParentDirectoryExists(_file_name);
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
		EnsureParentDirectoryExists(_file_name);
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
