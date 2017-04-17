#include "JsonValue.h"

using namespace common;
using std::string;

bool JsonValue::parse(const string& buf)
{
    Json::Reader jreader;
    if (jreader.parse(buf, jvalue))
    {   
        return true;
    }   
    return false;
}

string JsonValue::toString()
{
    Json::FastWriter jwriter; 
    std::string jstring = jwriter.write(jvalue);
    size_t size = jstring.size();
    if (size > 0 && jstring[size-1] == '\n')
        jstring.erase(size - 1); 
    return jstring;
}

string JsonValue::toFormatString()
{
    Json::StyledWriter jwriter; 
    std::string jstring = jwriter.write(jvalue);
    size_t size = jstring.size();
    if (size > 0 && jstring[size-1] == '\n')
        jstring.erase(size - 1); 
    return jstring;
}