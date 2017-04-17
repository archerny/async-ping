#ifndef _JSON_VALUE_H_
#define _JSON_VALUE_H_

#include <string>
#include "json/json.h"

using std::string;

namespace common
{
class JsonValue
{
public:
    bool parse(const string& buf);
    
    inline Json::Value& getValue()
    {   
        return jvalue;
    }   

    string toString();
    string toFormatString();

private:
    Json::Value  jvalue;
};

}

#endif