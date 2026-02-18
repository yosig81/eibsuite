#include <gtest/gtest.h>
#include "LogFile.h"
#include "CString.h"
#include "CException.h"
#include "../fixtures/TestHelpers.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <unistd.h>

using namespace EIBStdLibTest;

namespace {
std::string g_captured;

int CapturePrinter(const char* format, ...) {
    char buffer[1024];
    buffer[0] = '\0';

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    g_captured += buffer;
    return static_cast<int>(strlen(buffer));
}

CString MakeTempLogPath(const char* pattern) {
    char temp_template[96] = {0};
    strncpy(temp_template, pattern, sizeof(temp_template) - 1);
    int fd = mkstemp(temp_template);
    EXPECT_NE(-1, fd);
    if (fd != -1) {
        close(fd);
        unlink(temp_template);
    }
    return CString(temp_template);
}

std::string ReadAll(const CString& path) {
    std::ifstream in(path.GetBuffer(), std::ios::in);
    if (!in.is_open()) {
        return std::string();
    }

    std::string data;
    std::getline(in, data, '\0');
    return data;
}
}  // namespace

class LogFileTest : public BaseTestFixture {
protected:
    void SetUp() override {
        BaseTestFixture::SetUp();
        g_captured.clear();
    }
};

TEST_F(LogFileTest, ScreenTarget_PrintsWhenPromptEnabled) {
    CLogFile log;
    log.SetPrinterMethod(CapturePrinter);
    log.SetPrompt(true);

    log.Log(LOG_LEVEL_INFO, "screen message %d", 7);

    EXPECT_NE(std::string::npos, g_captured.find("screen message 7"));
}

TEST_F(LogFileTest, ScreenTarget_SuppressedWhenPromptDisabled) {
    CLogFile log;
    log.SetPrinterMethod(CapturePrinter);
    log.SetPrompt(false);

    log.Log(LOG_LEVEL_INFO, "hidden message");

    EXPECT_TRUE(g_captured.empty());
}

TEST_F(LogFileTest, LevelFiltering_AppliesToAllTargets) {
    CLogFile log;
    log.SetPrinterMethod(CapturePrinter);
    log.SetPrompt(true);
    log.SetLogLevel(LOG_LEVEL_ERROR);

    log.Log(LOG_LEVEL_INFO, "info ignored");
    log.Log(LOG_LEVEL_ERROR, "error kept");

    EXPECT_EQ(std::string::npos, g_captured.find("info ignored"));
    EXPECT_NE(std::string::npos, g_captured.find("error kept"));
}

TEST_F(LogFileTest, FileTarget_WritesWithoutPrinter) {
    CLogFile log;
    CString path = MakeTempLogPath("/tmp/eib_logfile_file_only_XXXXXX.log");

    log.Init(path);
    log.SetPrompt(false);
    log.Log(LOG_LEVEL_INFO, "file only message");

    std::string file_data = ReadAll(path);
    EXPECT_NE(std::string::npos, file_data.find("file only message"));
    EXPECT_NE(std::string::npos, file_data.find("Info:"));

    unlink(path.GetBuffer());
}

TEST_F(LogFileTest, BothTargets_WriteToCallbackAndFile) {
    CLogFile log;
    CString path = MakeTempLogPath("/tmp/eib_logfile_both_XXXXXX.log");

    log.Init(path);
    log.SetPrinterMethod(CapturePrinter);
    log.SetPrompt(true);
    log.Log(LOG_LEVEL_INFO, "both targets message");

    EXPECT_NE(std::string::npos, g_captured.find("both targets message"));

    std::string file_data = ReadAll(path);
    EXPECT_NE(std::string::npos, file_data.find("both targets message"));

    unlink(path.GetBuffer());
}

TEST_F(LogFileTest, CStringOverload_LogsMessage) {
    CLogFile log;
    log.SetPrinterMethod(CapturePrinter);
    log.SetPrompt(true);

    log.Log(LOG_LEVEL_INFO, CString("cstring overload"));

    EXPECT_NE(std::string::npos, g_captured.find("cstring overload"));
}

