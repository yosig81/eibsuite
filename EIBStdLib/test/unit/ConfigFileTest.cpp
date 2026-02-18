#include <gtest/gtest.h>
#include "ConfigFile.h"
#include "Globals.h"
#include "CException.h"
#include "../fixtures/TestHelpers.h"
#include <cstdio>
#include <fstream>
#include <string>
#include <unistd.h>

using namespace EIBStdLibTest;

class ConfigFileHarness : public CConfigFile {
public:
    bool Save(const CString& file_name) { return SaveToFile(file_name); }
    bool Load(const CString& file_name) { return LoadFromFile(file_name); }
    list<CConfigBlock>& Blocks() { return _conf; }

    bool ParseBool(const CString& value) {
        bool out = false;
        ParamFromString(value, out);
        return out;
    }

    int ParseInt(const CString& value) {
        int out = 0;
        ParamFromString(value, out);
        return out;
    }

    double ParseDouble(const CString& value) {
        double out = 0;
        ParamFromString(value, out);
        return out;
    }

    CString FormatInt(int value) {
        CString out;
        ParamToString(out, value);
        return out;
    }
};

class ConfigFileTest : public BaseTestFixture {};

TEST_F(ConfigFileTest, ConfigBlock_UpdateInsertsAndReplacesByName) {
    CConfigBlock block;
    block.SetName("main");

    CConfParam p1;
    p1.SetName("port");
    p1.SetValue("8080");
    block.Update(p1);
    ASSERT_EQ(1, static_cast<int>(block.GetParams().size()));
    EXPECT_STREQ("8080", block.GetParams().front().GetValueConst().GetBuffer());

    CConfParam p2;
    p2.SetName("port");
    p2.SetValue("9090");
    block.Update(p2);
    ASSERT_EQ(1, static_cast<int>(block.GetParams().size()));
    EXPECT_STREQ("9090", block.GetParams().front().GetValueConst().GetBuffer());
}

TEST_F(ConfigFileTest, ParamConversionHelpers_ParseAndFormatValues) {
    ConfigFileHarness cfg;
    EXPECT_TRUE(cfg.ParseBool("true"));
    EXPECT_EQ(42, cfg.ParseInt("42"));
    EXPECT_TRUE(FloatEquals(3.5, cfg.ParseDouble("3.5")));
    EXPECT_STREQ("77", cfg.FormatInt(77).GetBuffer());
}

TEST_F(ConfigFileTest, SaveAndLoad_RoundTripSingleBlock) {
    ConfigFileHarness out_cfg;

    CConfigBlock block;
    block.SetName("network");

    CConfParam host;
    host.SetName("host");
    host.SetValue("127.0.0.1");
    host.SetComments("# host comment");
    block.Update(host);

    CConfParam port;
    port.SetName("port");
    port.SetValue("3671");
    block.Update(port);

    out_cfg.Blocks().push_back(block);

    CString file_name("phase2_config_");
    file_name += static_cast<int>(getpid());
    file_name += ".cfg";

    ASSERT_TRUE(out_cfg.Save(file_name));

    ConfigFileHarness in_cfg;
    ASSERT_TRUE(in_cfg.Load(file_name));
    ASSERT_EQ(1, static_cast<int>(in_cfg.Blocks().size()));

    CConfigBlock loaded = in_cfg.Blocks().front();
    EXPECT_STREQ("network", loaded.GetName().GetBuffer());
    ASSERT_EQ(2, static_cast<int>(loaded.GetParams().size()));

    bool saw_host = false;
    bool saw_port = false;
    list<CConfParam>::iterator it;
    for (it = loaded.GetParams().begin(); it != loaded.GetParams().end(); ++it) {
        if (it->GetName() == "host") {
            saw_host = true;
            EXPECT_STREQ("127.0.0.1", it->GetValueConst().GetBuffer());
        } else if (it->GetName() == "port") {
            saw_port = true;
            EXPECT_STREQ("3671", it->GetValueConst().GetBuffer());
        }
    }
    EXPECT_TRUE(saw_host);
    EXPECT_TRUE(saw_port);

    CString path = CURRENT_CONF_FOLDER + file_name;
    std::remove(path.GetBuffer());
}

TEST_F(ConfigFileTest, Load_MalformedFileThrows) {
    if (!CDirectory::IsExist(CURRENT_CONF_FOLDER)) {
        ASSERT_TRUE(CDirectory::Create(CURRENT_CONF_FOLDER));
    }

    CString file_name("phase2_bad_config_");
    file_name += static_cast<int>(getpid());
    file_name += ".cfg";
    CString path = CURRENT_CONF_FOLDER + file_name;

    std::ofstream f(path.GetBuffer(), std::ios::out | std::ios::trunc);
    ASSERT_TRUE(f.good());
    f << "key=value" << std::endl;  // parameter outside [block]
    f.close();

    ConfigFileHarness cfg;
    EXPECT_THROW(cfg.Load(file_name), CEIBException);

    std::remove(path.GetBuffer());
}

TEST_F(ConfigFileTest, Load_CrlfLines_TrimmedWithoutTrailingCarriageReturn) {
    if (!CDirectory::IsExist(CURRENT_CONF_FOLDER)) {
        ASSERT_TRUE(CDirectory::Create(CURRENT_CONF_FOLDER));
    }

    CString file_name("phase2_crlf_config_");
    file_name += static_cast<int>(getpid());
    file_name += ".cfg";
    CString path = CURRENT_CONF_FOLDER + file_name;

    std::ofstream f(path.GetBuffer(), std::ios::out | std::ios::trunc | std::ios::binary);
    ASSERT_TRUE(f.good());
    f << "[network]\r\n";
    f << "listen_interface = en0\r\n";
    f << "port = 3671\r\n";
    f.close();

    ConfigFileHarness cfg;
    ASSERT_TRUE(cfg.Load(file_name));
    ASSERT_EQ(1, static_cast<int>(cfg.Blocks().size()));

    CConfigBlock loaded = cfg.Blocks().front();
    ASSERT_EQ(2, static_cast<int>(loaded.GetParams().size()));

    bool saw_if = false;
    list<CConfParam>::iterator it;
    for (it = loaded.GetParams().begin(); it != loaded.GetParams().end(); ++it) {
        if (it->GetName() == "listen_interface") {
            saw_if = true;
            EXPECT_STREQ("en0", it->GetValueConst().GetBuffer());
        }
    }
    EXPECT_TRUE(saw_if);

    std::remove(path.GetBuffer());
}
