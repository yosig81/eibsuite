#ifndef __XML_JSON_UTIL_HEADER__
#define __XML_JSON_UTIL_HEADER__

#include "CString.h"

class CXmlJsonUtil
{
public:
	// Convert ConsoleManager XML response to JSON string.
	// Handles the flat/shallow XML structures used by ConsoleManager:
	// - Elements with text content become JSON key/value pairs
	// - Repeated elements with same parent become JSON arrays
	// - Nested elements become JSON objects
	static CString XmlToJson(const CString& xml);

	// JSON string escaping
	static CString JsonEscape(const CString& str);

	// Build a simple JSON error object
	static CString JsonError(const CString& message);

	// Build a simple JSON success object
	static CString JsonOk();

private:
	static CString ParseElement(const char*& pos, const char* end);
	static CString ParseChildren(const char*& pos, const char* end);
	static void SkipWhitespace(const char*& pos, const char* end);
	static CString ReadTagName(const char*& pos, const char* end);
	static CString ReadTextContent(const char*& pos, const char* end);
	static bool IsClosingTag(const char* pos, const char* end, const CString& tag_name);
	static void SkipToAfterClosingTag(const char*& pos, const char* end, const CString& tag_name);
};

#endif
