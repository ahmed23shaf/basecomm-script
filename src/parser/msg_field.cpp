#include "msg_field.h"

MessageField::MessageField(std::string name, std::string type, std::string size, std::string detail, std::vector<std::string> defaultData)
    : m_name(name), m_type(type), m_size(std::stoi(size))
{
    m_typeEnum = stringToType(m_type);
    m_details = split(detail, '\n');

    if (!defaultData.empty())
    {
        for (std::string dataField : defaultData)
        {
            int hexResult;
            // Converting packet length to base-10 int [HEX->INT]
            std::istringstream converter(dataField);   
            converter >> std::hex >> hexResult;
            m_data.push_back(static_cast<BYTE>(hexResult));
        }
    }
    else
        m_data = std::vector<BYTE>(std::stoi(size), 0);
}

MessageField::MessageField(std::string name, std::string type, std::string size, std::string detail)
    : m_name(name), m_type(type), m_size(std::stoi(size))
{
    m_typeEnum = stringToType(m_type);
    m_details = split(detail, '\n');

    m_data = std::vector<BYTE>(std::stoi(size), 0);
}

std::vector<std::string> split(const std::string& str, const char& delimiter)
{
    std::vector<std::string> elems;
    std::stringstream ss(str);

    std::string currentElem;
    while (std::getline(ss, currentElem, delimiter))
    {
        elems.push_back(currentElem);
    }

    return elems;
}

std::string MessageField::getName()
{
    return m_name;
}
std::string MessageField::getType()
{
    return m_type;
}
MessageField::PacketItem_t MessageField::getTypeEnum()
{
    return m_typeEnum;
}
int     MessageField::getSize()
{
    return m_size;
}
std::vector<std::string> MessageField::getDetails()
{
    return m_details;
}

std::vector<BYTE> MessageField::getData()
{
    return m_data;
}

std::string MessageField::typeToString(PacketItem_t typeEnum)
{
    switch (typeEnum)
    {
        case bitfield:    return std::string("bitfield"     ); break;
        case byte:        return std::string("byte"         ); break;
        case word:        return std::string("word"         ); break;
        case string:      return std::string("string"       ); break;
        case longInt:     return std::string("long"         ); break;
        case signedWord:  return std::string("signed word"  ); break;
        case bytes:       return std::string("bytes"        ); break;
        case words:       return std::string("words"        ); break;
        case long_long:   return std::string("long long"    ); break;
        default:          return std::string("UNKNOWN"      ); break;
        }
}

MessageField::PacketItem_t MessageField::stringToType(std::string type)
{
    if      (type == "bitfield")    { return bitfield; }
    else if (type == "byte")        { return byte; }
    else if (type == "word")        { return word; }
    else if (type == "string")      { return string; }
    else if (type == "long")        { return longInt; }
    else if (type == "signed word") { return signedWord; }
    else if (type == "bytes")       { return bytes; }
    else if (type == "words")       { return words; }
    else if (type == "long long")   { return long_long; }
    else                            { return none; }
}

void MessageField::printMsgField()
{
    std::cout << m_name << " " << m_type << " " << m_size << '\n' ;
    for (std::string& detail : m_details) std::cout << detail << '\n';
    for (BYTE& byte : m_data)             std::cout << std::uppercase << std::setw(2) << 
                                            std::setfill('0') << std::hex << (int)(byte) << ", ";
    std::cout << std::dec;
    std::cout << '\n';

}
