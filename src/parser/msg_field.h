#ifndef MSG_FIELD_H
#define MSG_FIELD_H

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <string>
#include <vector>
#include <sstream>
#include <cstdint>

typedef unsigned int uint;
typedef uint8_t BYTE;

std::vector<std::string> split(const std::string& str, const char& delimiter); // Helper, splits a string using delimiter                        

/**
 * @brief The MessageField class contains all necessary parsing info for
 * an individual data item inside a packet. ONLY ONE ITEM, such as a
 * single int, or a single byte. There are often multiple MessageDataItems
 * transmitted inside each packet.
 *
 * The MessageField class is the blueprint for each message item, holding data 
 * bytes inside `m_data`
 * 
 * This class holds the data info and actual bytes for a field. 
 * ex. Consider MDB_Rpm_Command, an object of this class would be:
 * m_name: "rpm command"
 * m_type: word, m_size: 2 bytes, m_details: 100 - 7000 rpm, m_data: 0x0000 (as a byte array)
 * 
 */
class MessageField
{
    public:
        MessageField(std::string name, std::string type, std::string size, std::string detail, std::vector<std::string> defaultData);
        MessageField(std::string name, std::string type, std::string size, std::string detail);

        enum PacketItem_t {
            none = 0,
            bitfield,
            byte,
            word,
            string,
            longInt,
            signedWord,
            bytes,
            words,
            long_long,
            maxTypes
        };

        // Getters
        std::string                 getName();
        std::string                 getType();
        PacketItem_t                getTypeEnum();
        int                         getSize();
        std::vector<std::string>    getDetails();
        std::vector<BYTE>           getData();

        // Setters
        void setName(std::string name)                      {m_name = name;}
        void setType(std::string type)                      {m_type = type;     m_typeEnum = stringToType(type); }
        void setType(PacketItem_t type)                     {m_typeEnum = type; m_type = typeToString(type); }
        void setSize(int size)                              {m_size = size;}
        void setDetails(std::vector<std::string> details)   {m_details = details;}
        void setData(std::vector<BYTE> data)                {m_data = data;}

        static std::string typeToString(PacketItem_t typeEnum);
        static PacketItem_t stringToType(std::string type);

        void printMsgField();
    private:
        std::string                m_name;         // Human name of data item
        std::string                m_type;         // Human readable string for type
        PacketItem_t               m_typeEnum;     // enum version for type
        int                        m_size;         // In bytes
        std::vector<std::string>   m_details;      // Info about the data

        std::vector<BYTE>          m_data;         // Actual data contents (if any)
        // m_data[0] represents the byte closest to the header bytes.
        // m_data[m_data.size()-1] represents the byte closest to checksum. 
};

#endif // MSG_FIELD_H
