// XmlParserTest.cpp -- Tests for CXmlDocument parsing, especially
// when the source buffer is a CDataBuffer (not null-terminated).

#include <gtest/gtest.h>
#include "xml/Xml.h"
#include "DataBuffer.h"
#include "CString.h"
#include "../fixtures/TestHelpers.h"
#include <cstring>
#include <string>

using namespace EIBStdLibTest;

class XmlParserTest : public BaseTestFixture {};

// -----------------------------------------------------------------------
// Basic parsing
// -----------------------------------------------------------------------

TEST_F(XmlParserTest, ParseFromNullTerminatedString) {
    const char* xml = "<?xml version=\"1.0\"?><Root><Name>test</Name></Root>";

    CXmlDocument doc(xml);
    CXmlElement root = doc.RootElement();
    ASSERT_TRUE(root.IsValid());

    CXmlElement name = root.FirstChildElement("Name");
    ASSERT_TRUE(name.IsValid());
    EXPECT_STREQ("test", name.GetValue().GetBuffer());
}

TEST_F(XmlParserTest, ParseFromCString) {
    CString xml("<?xml version=\"1.0\"?><Root><Item>hello</Item></Root>");

    CXmlDocument doc;
    doc.Parse(xml);

    CXmlElement root = doc.RootElement();
    ASSERT_TRUE(root.IsValid());

    CXmlElement item = root.FirstChildElement("Item");
    ASSERT_TRUE(item.IsValid());
    EXPECT_STREQ("hello", item.GetValue().GetBuffer());
}

// -----------------------------------------------------------------------
// CDataBuffer -> CString -> Parse (the pattern fixed in FromXml)
// -----------------------------------------------------------------------

TEST_F(XmlParserTest, ParseFromDataBuffer_ViaCString) {
    // This tests the exact pattern used in the FromXml fix:
    //   CString xml_safe((const char*)buf.GetBuffer(), buf.GetLength());
    //   _doc.Parse(xml_safe);
    const char* xml = "<?xml version=\"1.0\"?><Root><User>admin</User></Root>";
    int xml_len = static_cast<int>(std::strlen(xml));

    CDataBuffer buf;
    buf.Add(xml, xml_len);
    // CDataBuffer is NOT null-terminated -- buf.GetBuffer()[xml_len] is undefined.

    // Safe: create CString from buffer (guarantees null termination)
    CString xml_safe((const char*)buf.GetBuffer(), buf.GetLength());

    CXmlDocument doc;
    doc.Parse(xml_safe);

    CXmlElement root = doc.RootElement();
    ASSERT_TRUE(root.IsValid());

    CXmlElement user = root.FirstChildElement("User");
    ASSERT_TRUE(user.IsValid());
    EXPECT_STREQ("admin", user.GetValue().GetBuffer());
}

TEST_F(XmlParserTest, ParseFromDataBuffer_LargePayload) {
    // Build an XML string large enough to force CDataBuffer reallocation
    // (> 256 bytes, the default inline buffer size).
    std::string xml = "<?xml version=\"1.0\"?><Root><List>";
    for (int i = 0; i < 20; i++) {
        xml += "<Item><Name>user" + std::to_string(i) + "</Name>";
        xml += "<Value>" + std::to_string(i * 100) + "</Value></Item>";
    }
    xml += "</List></Root>";

    ASSERT_GT(xml.size(), 256u)
        << "Payload must exceed CDataBuffer inline storage to test reallocation";

    CDataBuffer buf;
    buf.Add(xml.c_str(), static_cast<int>(xml.size()));

    // Verify CDataBuffer was reallocated (not using inline storage)
    EXPECT_GT(buf.GetAllocated(), DATA_BUFFER_DEFAULT_SIZE);

    // Parse via CString (the safe pattern)
    CString xml_safe((const char*)buf.GetBuffer(), buf.GetLength());
    CXmlDocument doc;
    doc.Parse(xml_safe);

    CXmlElement root = doc.RootElement();
    ASSERT_TRUE(root.IsValid());

    CXmlElement list = root.FirstChildElement("List");
    ASSERT_TRUE(list.IsValid());

    CXmlElement first_item = list.FirstChildElement("Item");
    ASSERT_TRUE(first_item.IsValid());

    CXmlElement name = first_item.FirstChildElement("Name");
    ASSERT_TRUE(name.IsValid());
    EXPECT_STREQ("user0", name.GetValue().GetBuffer());
}

// -----------------------------------------------------------------------
// ToXml / Parse round-trip via CDataBuffer
// -----------------------------------------------------------------------

TEST_F(XmlParserTest, ToStringAndReparse_RoundTrip) {
    // Build a document, serialize to CDataBuffer, reparse from it.
    CXmlDocument doc1;
    CXmlElement root1 = doc1.RootElement();
    ASSERT_TRUE(root1.IsValid());

    CXmlElement child = root1.InsertChild("Data");
    child.SetValue("42");

    CDataBuffer xml_buf;
    doc1.ToString(xml_buf);
    ASSERT_GT(xml_buf.GetLength(), 0);

    // Reparse using the safe CString pattern
    CString xml_safe((const char*)xml_buf.GetBuffer(), xml_buf.GetLength());
    CXmlDocument doc2;
    doc2.Parse(xml_safe);

    CXmlElement root2 = doc2.RootElement();
    ASSERT_TRUE(root2.IsValid());

    CXmlElement data = root2.FirstChildElement("Data");
    ASSERT_TRUE(data.IsValid());
    EXPECT_STREQ("42", data.GetValue().GetBuffer());
}

// -----------------------------------------------------------------------
// Multiple sibling elements (the pattern used in users XML)
// -----------------------------------------------------------------------

TEST_F(XmlParserTest, ParseMultipleSiblings) {
    const char* xml =
        "<?xml version=\"1.0\"?>"
        "<Root><UserList>"
        "<User><Name>alice</Name><Role>admin</Role></User>"
        "<User><Name>bob</Name><Role>reader</Role></User>"
        "<User><Name>carol</Name><Role>writer</Role></User>"
        "</UserList></Root>";

    CString xml_str(xml);
    CXmlDocument doc;
    doc.Parse(xml_str);

    CXmlElement root = doc.RootElement();
    ASSERT_TRUE(root.IsValid());

    CXmlElement list = root.FirstChildElement("UserList");
    ASSERT_TRUE(list.IsValid());

    // Iterate siblings
    int count = 0;
    CXmlElement user = list.FirstChildElement("User");
    while (user.IsValid()) {
        count++;
        user = user.NextSibling("User");
    }
    EXPECT_EQ(3, count);

    // Verify first element
    CXmlElement first = list.FirstChildElement("User");
    ASSERT_TRUE(first.IsValid());
    EXPECT_STREQ("alice", first.FirstChildElement("Name").GetValue().GetBuffer());
    EXPECT_STREQ("admin", first.FirstChildElement("Role").GetValue().GetBuffer());
}
