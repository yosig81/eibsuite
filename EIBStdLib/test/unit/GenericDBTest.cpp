#include <gtest/gtest.h>
#include "GenericDB.h"
#include "CString.h"
#include "CException.h"
#include "../fixtures/TestHelpers.h"
#include <cstdio>
#include <fstream>
#include <list>
#include <unistd.h>

using namespace EIBStdLibTest;

namespace {
struct GenericRecord {
    CString key;
    int value;
    bool enabled;
    CString text;

    GenericRecord() : value(0), enabled(false) {}
};

class TestGenericDB : public CGenericDB<CString, GenericRecord> {
public:
    void SetFileWithoutCreate(const CString& file_name) { _file_name = file_name; }

    void OnReadParamComplete(GenericRecord& current_record, const CString& param, const CString& value) override {
        if (param == "value") {
            current_record.value = value.ToInt();
        } else if (param == "enabled") {
            current_record.enabled = value.ToBool();
        } else if (param == "text") {
            current_record.text = value;
        }
    }

    void OnReadRecordComplete(GenericRecord& current_record) override {
        AddRecord(current_record.key, current_record);
        current_record = GenericRecord();
    }

    void OnReadRecordNameComplete(GenericRecord& current_record, const CString& record_name) override {
        current_record = GenericRecord();
        current_record.key = record_name;
    }

    void OnSaveRecordStarted(
        const GenericRecord& record, CString& record_name,
        std::list<std::pair<CString, CString> >& param_values) override {
        record_name = record.key;
        param_values.push_back(std::make_pair(CString("value"), CString(record.value)));
        param_values.push_back(std::make_pair(CString("enabled"), CString(record.enabled ? "true" : "false")));
        param_values.push_back(std::make_pair(CString("text"), record.text));
    }
};

CString MakeTempPath(const char* pattern) {
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

void WriteFile(const CString& path, const char* data) {
    std::ofstream file(path.GetBuffer(), std::ios::out | std::ios::trunc);
    ASSERT_TRUE(file.is_open());
    file << data;
}
}  // namespace

class GenericDBTest : public BaseTestFixture {};

TEST_F(GenericDBTest, AddGetEditDelete_RecordLifecycle) {
    TestGenericDB db;

    GenericRecord rec;
    rec.key = "alpha";
    rec.value = 10;
    rec.enabled = true;
    rec.text = "hello";

    EXPECT_TRUE(db.AddRecord(rec.key, rec));
    EXPECT_FALSE(db.AddRecord(rec.key, rec));

    GenericRecord loaded;
    ASSERT_TRUE(db.GetRecord("alpha", loaded));
    EXPECT_EQ(10, loaded.value);
    EXPECT_TRUE(loaded.enabled);
    EXPECT_STREQ("hello", loaded.text.GetBuffer());

    rec.value = 42;
    rec.enabled = false;
    EXPECT_TRUE(db.EditRecord("alpha", rec));

    ASSERT_TRUE(db.GetRecord("alpha", loaded));
    EXPECT_EQ(42, loaded.value);
    EXPECT_FALSE(loaded.enabled);

    EXPECT_TRUE(db.DeleteRecord("alpha"));
    EXPECT_FALSE(db.DeleteRecord("alpha"));
    EXPECT_FALSE(db.GetRecord("alpha", loaded));
    EXPECT_TRUE(db.IsEmpty());
}

TEST_F(GenericDBTest, Init_CreatesMissingFile) {
    TestGenericDB db;
    CString path = MakeTempPath("/tmp/eib_genericdb_init_XXXXXX");

    EXPECT_EQ(-1, access(path.GetBuffer(), F_OK));
    db.Init(path);
    EXPECT_EQ(0, access(path.GetBuffer(), F_OK));

    unlink(path.GetBuffer());
}

TEST_F(GenericDBTest, SaveAndLoad_RoundTripMultipleRecords) {
    TestGenericDB db;
    CString path = MakeTempPath("/tmp/eib_genericdb_roundtrip_XXXXXX");
    db.Init(path);

    GenericRecord a;
    a.key = "first";
    a.value = 1;
    a.enabled = true;
    a.text = "alpha";
    ASSERT_TRUE(db.AddRecord(a.key, a));

    GenericRecord b;
    b.key = "second";
    b.value = 2;
    b.enabled = false;
    b.text = "beta";
    ASSERT_TRUE(db.AddRecord(b.key, b));

    ASSERT_TRUE(db.Save());

    TestGenericDB loaded_db;
    loaded_db.Init(path);
    ASSERT_TRUE(loaded_db.Load());
    EXPECT_EQ(2, loaded_db.GetNumOfRecords());

    GenericRecord loaded_a;
    ASSERT_TRUE(loaded_db.GetRecord("first", loaded_a));
    EXPECT_EQ(1, loaded_a.value);
    EXPECT_TRUE(loaded_a.enabled);
    EXPECT_STREQ("alpha", loaded_a.text.GetBuffer());

    GenericRecord loaded_b;
    ASSERT_TRUE(loaded_db.GetRecord("second", loaded_b));
    EXPECT_EQ(2, loaded_b.value);
    EXPECT_FALSE(loaded_b.enabled);
    EXPECT_STREQ("beta", loaded_b.text.GetBuffer());

    unlink(path.GetBuffer());
}

TEST_F(GenericDBTest, Load_ParamBeforeAnyRecord_Throws) {
    TestGenericDB db;
    CString path = MakeTempPath("/tmp/eib_genericdb_err_empty_brackets_XXXXXX");
    WriteFile(path, "value = 7\n");
    db.SetFileWithoutCreate(path);

    EXPECT_THROW(db.Load(), CEIBException);
    unlink(path.GetBuffer());
}

TEST_F(GenericDBTest, Load_MalformedBlockLine_Throws) {
    TestGenericDB db;
    CString path = MakeTempPath("/tmp/eib_genericdb_err_block_XXXXXX");
    WriteFile(path, "[broken\nvalue = 7\n");
    db.SetFileWithoutCreate(path);

    EXPECT_THROW(db.Load(), CEIBException);
    unlink(path.GetBuffer());
}

TEST_F(GenericDBTest, Load_MissingEquals_Throws) {
    TestGenericDB db;
    CString path = MakeTempPath("/tmp/eib_genericdb_err_equals_XXXXXX");
    WriteFile(path, "[rec]\nvalue 7\n");
    db.SetFileWithoutCreate(path);

    EXPECT_THROW(db.Load(), CEIBException);
    unlink(path.GetBuffer());
}

TEST_F(GenericDBTest, Load_MissingValue_Throws) {
    TestGenericDB db;
    CString path = MakeTempPath("/tmp/eib_genericdb_err_value_XXXXXX");
    WriteFile(path, "[rec]\nvalue =\n");
    db.SetFileWithoutCreate(path);

    EXPECT_THROW(db.Load(), CEIBException);
    unlink(path.GetBuffer());
}

TEST_F(GenericDBTest, EditAndDeleteMissingRecord_ReturnFalse) {
    TestGenericDB db;
    GenericRecord rec;
    rec.key = "missing";
    rec.value = 3;
    rec.enabled = true;
    rec.text = "x";

    EXPECT_FALSE(db.EditRecord("missing", rec));
    EXPECT_FALSE(db.DeleteRecord("missing"));
    EXPECT_TRUE(db.IsEmpty());
}

TEST_F(GenericDBTest, ClearAndGetNumOfRecords_Work) {
    TestGenericDB db;

    GenericRecord a;
    a.key = "a";
    ASSERT_TRUE(db.AddRecord(a.key, a));
    GenericRecord b;
    b.key = "b";
    ASSERT_TRUE(db.AddRecord(b.key, b));

    EXPECT_EQ(2, db.GetNumOfRecords());
    db.Clear();
    EXPECT_EQ(0, db.GetNumOfRecords());
    EXPECT_TRUE(db.IsEmpty());
}

TEST_F(GenericDBTest, Load_MissingFile_Throws) {
    TestGenericDB db;
    CString path = MakeTempPath("/tmp/eib_genericdb_missing_XXXXXX");
    unlink(path.GetBuffer());
    db.SetFileWithoutCreate(path);

    EXPECT_THROW(db.Load(), CEIBException);
}

TEST_F(GenericDBTest, Save_InvalidPath_ReturnsFalse) {
    TestGenericDB db;
    db.SetFileWithoutCreate("/tmp/eib_genericdb_no_such_dir/records.conf");

    GenericRecord rec;
    rec.key = "x";
    rec.value = 1;
    ASSERT_TRUE(db.AddRecord(rec.key, rec));

    EXPECT_FALSE(db.Save());
}

TEST_F(GenericDBTest, Load_CommentsWhitespaceAndCrlf_AreHandled) {
    TestGenericDB db;
    CString path = MakeTempPath("/tmp/eib_genericdb_whitespace_XXXXXX");
    WriteFile(
        path,
        "# comment\r\n"
        "   [rec]\r\n"
        " value =  9  \r\n"
        " enabled = true \r\n"
        " text = hello world \r\n");
    db.SetFileWithoutCreate(path);

    ASSERT_TRUE(db.Load());
    EXPECT_EQ(1, db.GetNumOfRecords());

    GenericRecord rec;
    ASSERT_TRUE(db.GetRecord("rec", rec));
    EXPECT_EQ(9, rec.value);
    EXPECT_TRUE(rec.enabled);
    EXPECT_STREQ("hello world", rec.text.GetBuffer());

    unlink(path.GetBuffer());
}
