#include "XmlJsonUtil.h"
#include <map>
#include <vector>
#include <string.h>

using namespace std;

CString CXmlJsonUtil::JsonEscape(const CString& str)
{
	CString result;
	for (int i = 0; i < str.GetLength(); i++) {
		char c = str[i];
		switch (c) {
		case '"':  result += "\\\""; break;
		case '\\': result += "\\\\"; break;
		case '\n': result += "\\n"; break;
		case '\r': result += "\\r"; break;
		case '\t': result += "\\t"; break;
		default:   result += c; break;
		}
	}
	return result;
}

CString CXmlJsonUtil::JsonError(const CString& message)
{
	CString result = "{\"error\":\"";
	result += JsonEscape(message);
	result += "\"}";
	return result;
}

CString CXmlJsonUtil::JsonOk()
{
	return "{\"status\":\"ok\"}";
}

void CXmlJsonUtil::SkipWhitespace(const char*& pos, const char* end)
{
	while (pos < end && (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n')) {
		pos++;
	}
}

CString CXmlJsonUtil::ReadTagName(const char*& pos, const char* end)
{
	CString name;
	// Skip '<'
	pos++;
	while (pos < end && *pos != '>' && *pos != ' ' && *pos != '/' && *pos != '?') {
		name += *pos;
		pos++;
	}
	// Skip to end of opening tag (past attributes and '>')
	while (pos < end && *pos != '>') {
		pos++;
	}
	if (pos < end) pos++; // skip '>'
	return name;
}

CString CXmlJsonUtil::ReadTextContent(const char*& pos, const char* end)
{
	CString text;
	while (pos < end && *pos != '<') {
		text += *pos;
		pos++;
	}
	return text;
}

bool CXmlJsonUtil::IsClosingTag(const char* pos, const char* end, const CString& tag_name)
{
	if (pos + 2 >= end) return false;
	if (pos[0] != '<' || pos[1] != '/') return false;

	const char* p = pos + 2;
	for (int i = 0; i < tag_name.GetLength() && p < end; i++, p++) {
		if (*p != tag_name[i]) return false;
	}
	return (p < end && *p == '>');
}

void CXmlJsonUtil::SkipToAfterClosingTag(const char*& pos, const char* end, const CString& tag_name)
{
	// Skip "</tagname>"
	pos += 2; // skip "</"
	pos += tag_name.GetLength(); // skip tag name
	if (pos < end && *pos == '>') pos++; // skip '>'
}

CString CXmlJsonUtil::XmlToJson(const CString& xml)
{
	if (xml.GetLength() == 0) {
		return "{}";
	}

	const char* pos = xml.GetBuffer();
	const char* end = pos + xml.GetLength();

	SkipWhitespace(pos, end);

	// Skip XML declaration if present
	if (pos < end - 1 && pos[0] == '<' && pos[1] == '?') {
		while (pos < end - 1) {
			if (pos[0] == '?' && pos[1] == '>') {
				pos += 2;
				break;
			}
			pos++;
		}
		SkipWhitespace(pos, end);
	}

	// Parse root element
	if (pos >= end || *pos != '<') {
		return "{}";
	}

	return ParseElement(pos, end);
}

CString CXmlJsonUtil::ParseElement(const char*& pos, const char* end)
{
	SkipWhitespace(pos, end);
	if (pos >= end || *pos != '<' || pos[1] == '/') {
		return "{}";
	}

	CString tag_name = ReadTagName(pos, end);

	SkipWhitespace(pos, end);

	// Check if immediately followed by closing tag (empty element)
	if (pos < end && IsClosingTag(pos, end, tag_name)) {
		SkipToAfterClosingTag(pos, end, tag_name);
		return "{}";
	}

	// Check if text content (no child elements)
	if (pos < end && *pos != '<') {
		CString text = ReadTextContent(pos, end);
		if (pos < end && IsClosingTag(pos, end, tag_name)) {
			SkipToAfterClosingTag(pos, end, tag_name);
			// Return just the text value (caller wraps in object)
			CString result = "\"";
			result += JsonEscape(text);
			result += "\"";
			return result;
		}
	}

	// Has child elements - parse them
	return ParseChildren(pos, end);
}

CString CXmlJsonUtil::ParseChildren(const char*& pos, const char* end)
{
	// Collect all child elements, tracking duplicates for arrays
	typedef vector<CString> ValueList;
	typedef map<CString, ValueList> ChildMap;

	// We need to preserve order, so keep a separate order list
	vector<CString> key_order;
	ChildMap children;

	while (pos < end) {
		SkipWhitespace(pos, end);
		if (pos >= end) break;

		// Check for closing tag of parent
		if (*pos == '<' && pos + 1 < end && pos[1] == '/') {
			// Skip this closing tag
			while (pos < end && *pos != '>') pos++;
			if (pos < end) pos++;
			break;
		}

		if (*pos != '<') {
			pos++;
			continue;
		}

		// Read tag name
		CString child_tag = ReadTagName(pos, end);

		SkipWhitespace(pos, end);

		// Check for self-closing or empty element
		if (pos < end && IsClosingTag(pos, end, child_tag)) {
			SkipToAfterClosingTag(pos, end, child_tag);
			if (children.find(child_tag) == children.end()) {
				key_order.push_back(child_tag);
			}
			children[child_tag].push_back("\"\"");
			continue;
		}

		// Text content?
		if (pos < end && *pos != '<') {
			CString text = ReadTextContent(pos, end);
			if (pos < end && IsClosingTag(pos, end, child_tag)) {
				SkipToAfterClosingTag(pos, end, child_tag);
				CString val = "\"";
				val += JsonEscape(text);
				val += "\"";
				if (children.find(child_tag) == children.end()) {
					key_order.push_back(child_tag);
				}
				children[child_tag].push_back(val);
				continue;
			}
		}

		// Nested element with children
		CString nested = ParseChildren(pos, end);
		if (children.find(child_tag) == children.end()) {
			key_order.push_back(child_tag);
		}
		children[child_tag].push_back(nested);
	}

	// Build JSON object
	CString result = "{";
	bool first = true;
	for (size_t i = 0; i < key_order.size(); i++) {
		const CString& key = key_order[i];
		const ValueList& vals = children[key];

		if (!first) result += ",";
		first = false;

		result += "\"";
		result += JsonEscape(key);
		result += "\":";

		if (vals.size() == 1) {
			result += vals[0];
		} else {
			result += "[";
			for (size_t j = 0; j < vals.size(); j++) {
				if (j > 0) result += ",";
				result += vals[j];
			}
			result += "]";
		}
	}
	result += "}";

	return result;
}
