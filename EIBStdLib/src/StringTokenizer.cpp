#include "StringTokenizer.h"

StringTokenizer::StringTokenizer(const CString& str, const CString& delim)
{

   if ((str.GetLength() == 0) || (delim.GetLength() == 0)) return;

   _token_str = str;
   _delim     = delim;

  /*
     Remove sequential delimiter
  */
   int curr_pos = 0;

   while(true)
   {
      if ((curr_pos = _token_str.Find(delim,curr_pos)) != static_cast<int>(string::npos))
      {
         curr_pos += _delim.GetLength();

         while(_token_str.Find(_delim,curr_pos) == curr_pos)
         {
            _token_str.Erase(curr_pos,_delim.GetLength());
         }
      }
      else
       break;
   }

   /*
     Trim leading delimiter
   */
   if (_token_str.Find(_delim,0) == 0)
   {
      _token_str.Erase(0,_delim.GetLength());
   }

   /*
     Trim ending delimiter
   */
   curr_pos = 0;
   if ((curr_pos = _token_str.RFind(_delim)) != static_cast<int>(string::npos))
   {
      if (curr_pos != (_token_str.GetLength() - _delim.GetLength())) return;
	  _token_str.Erase(_token_str.GetLength() - _delim.GetLength(),_delim.GetLength());
   }

}


int StringTokenizer::CountTokens()
{
   if (_token_str.GetLength() == 0)
      return 0;

   int num_tokens = 1; // At least one token if string is non-empty
   int curr_pos = 0;

   while(true)
   {
      int found_pos = _token_str.Find(_delim, curr_pos);
      if (found_pos == -1) // Not found
         break;

      num_tokens++;
      curr_pos = found_pos + _delim.GetLength();

      // Prevent infinite loop if we're at or past the end
      if (curr_pos >= (int)_token_str.GetLength())
         break;
   }

   return num_tokens;
}


bool StringTokenizer::HasMoreTokens()
{
   return (_token_str.GetLength() > 0);
}


CString StringTokenizer::NextToken()
{

   if (_token_str.GetLength() == 0)
     return "";

   CString  tmp_str = "";
   int pos = _token_str.Find(_delim,0); // Changed to int to properly handle -1

   if (pos != -1 && pos != static_cast<int>(string::npos))
   {
      tmp_str   = _token_str.SubString(0,pos);
      int newStart = pos + _delim.GetLength();
      int newLength = _token_str.GetLength() - pos - _delim.GetLength();
      _token_str = _token_str.SubString(newStart, newLength);
   }
   else
   {
      tmp_str   = _token_str.SubString(0,_token_str.GetLength());
      _token_str.Clear();
   }

   return tmp_str;
}


int StringTokenizer::NextIntToken()
{
   return NextToken().ToInt();
}

int64 StringTokenizer::NextInt64Token()
{
	
	return NextToken().ToInt64();
}

double StringTokenizer::NextFloatToken()
{
   return NextToken().ToDouble();
}


CString StringTokenizer::NextToken(const CString& delimiter)
{
   if (_token_str.GetLength() == 0)
     return "";

   CString  tmp_str = "";
   int pos = _token_str.Find(delimiter,0); // Changed to int to properly handle -1

   if (pos != -1 && pos != static_cast<int>(string::npos))
   {
      tmp_str   = _token_str.SubString(0,pos);
      int newStart = pos + delimiter.GetLength();
      int newLength = _token_str.GetLength() - pos - delimiter.GetLength();
	  _token_str = _token_str.SubString(newStart, newLength);
   }
   else
   {
      tmp_str   = _token_str.SubString(0,_token_str.GetLength());
      _token_str = "";
   }

   return tmp_str;
}


CString StringTokenizer::RemainingString()
{
   return _token_str;
}


CString StringTokenizer::FilterNextToken(const CString& filterStr)
{
   CString  tmp_str = NextToken();
   unsigned int currentPos = 0;

   while((currentPos = tmp_str.Find(filterStr,currentPos)) != string::npos)
   {
      tmp_str.Erase(currentPos,filterStr.GetLength());
   }

   return tmp_str;
}
