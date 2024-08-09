#ifndef MSG_H
#define MSG_H

#include "msg_field.h"
#include "../lib/pugi/pugixml.hpp"
#include "../serial/comm_errors.h"
#include <string>
#include <cstring>

#define MIN_PACK_LEN 6

using namespace pugi;

/**
 * @brief The Message class is the blueprint of any sent (TX) or received (RX)
 * packet through the UART cable. The representation of each command or report
 * follows a pattern detailed at the end of this file. A Message object uses
 * multiple MessageField containers to hold the data.
 * 
 * Each message can be broken down as shown below with '|' seperating byte(s):
 * 
 * TARGET ID | PACKET LENGTH | SEQUENCE NUMBER | SENDER ID | MSG VALUE (IDENTIFIER) | DATA BYTES | CHECKSUM
 *
 * The first 5 bytes of a message is known as the 'header'
 */

class Message
{
    public:
        Message(xml_node commandNode);

        // Object-based function for sending a message
        // @return a type of error, see serial/comm_errors.h, usually 0 on success
        comm_error sendMessage();

        // Getters
        std::string getSearchTag()              { return m_SearchTag    ; }
        std::string getMsgName()                { return m_MsgName      ; }
        std::string getTargetID()               { return m_TargetID     ; }
        std::string getSenderID()               { return m_SenderID     ; }
        std::string getMsgValue()               { return m_MsgValue     ; }
        int         getPackLen()                { return m_PackLen      ; }
        bool        getIsBootModeCmd()          { return m_IsBootModeCmd; }
        std::string getDuplicateCmd()           { return m_DuplicateCmd ; }
        std::string getCmdNotes()               { return m_CmdNotes     ; }
        std::vector<MessageField*>& getDataFormat()   { return data_format; }

        std::vector<BYTE> getHeader();
        std::vector<BYTE> getDataBuffer();                      // data bytes
        BYTE              getChecksum();
        std::vector<BYTE> getMessageBuffer();                   // includes all bytes within the message

        MessageField* findMessageField(std::string fieldName);  // returns NULL if not found and prints ERROR message


        // Formatted Strings
        std::string getMsgLengthStr()       { return std::to_string(m_PackLen); }
        std::string getMsgDirectionStr();
        std::string getMsgFieldCountStr()   { return std::to_string(data_format.size()); }

        // Setters
        void setSearchTag     (std::string SearchTag    );
        void setTargetID      (std::string TargetID     );
        void setSenderID      (std::string SenderID     );
        void setMsgValue      (std::string MsgValue     );
        void setMsgName       (std::string MsgName      ) {      m_MsgName  = MsgName      ; }
        void setPackLen       (int     PackLen      )     {      m_PackLen  = PackLen      ; }
        void setIsBootModeCmd (bool    IsBootModeCmd)     {m_IsBootModeCmd  = IsBootModeCmd; }
        void setDuplicateCmd  (std::string DuplicateCmd ) { m_DuplicateCmd  = DuplicateCmd ; }
        void setCmdNotes      (std::string CmdNotes     ) {     m_CmdNotes  = CmdNotes     ; }

        void setDataBuffer    (const std::vector<BYTE>& newBuffer);

        bool isOutgoing()   { return m_SenderID == std::string("F0");}  // 0xF0 is the ID for console
        bool isEditable()   { return this->isOutgoing() && this->getDataBuffer().size() != 0; }

        /**
         * Sets the value of a field identified by `dataName` to the value provided in `input`.
         * The field type `T` is later casted to the type of DataName with the use of helper functions.
         * 
         * @tparam T The type of the input value.
         * @param dataName The identifier of the data field to be set.
         * @param input The value to set for the data field.
         * @return true if the field was successfully set, false otherwise.
         */
        template <typename T>
        bool setField(const std::string& dataName, const T& input);

        template <typename T>
        T getField(const std::string& dataName);

        void printMsg();
    private:
        std::string m_SearchTag;            // DESTINATION:SOURCE:MSG_CODE
        std::string m_TargetID;             // Destination ID
        std::string m_SenderID;             // Source ID
        std::string m_MsgValue;             // MSG Code (identifier)
        std::string m_MsgName;
        int         m_PackLen;              // Total # of bytes in the packet

        std::vector<MessageField*> data_format;

        bool        m_IsBootModeCmd;
        std::string m_DuplicateCmd;   //unused??
        std::string m_CmdNotes;       

        enum searchTagUpdate{ fromSearchTag, fromID };
        void updateSearchTag(searchTagUpdate type);
        std::string deviceIDtoStr(const std::string& id);
        bool checkSearchTag(std::string& st);
        bool checkByteInput(std::string& str);
};

/*
    SearchTag   - string
    TargetID    - hex byte
    SenderID    - hex byte
    MsgValue    - hex byte
    MsgName     - string
    PackLen     - int

    DataNames   - list of strings (csv)
    DataTypes   - list of strings (data types that match data names) (csv)
    DataSizes   - list of ints    (byte sizes of above data types) (csv)
    DataDetails - list of strings (details message for the above data) (csv) (newline separated bitfields)
    DefaultData - list of ints  (default values for above data) (csv)

    IsBootModeCmd - bool
    DupicateCmd
    CmdNotes
*/

// Grabs the first `num` bytes from `input`
// The returned vector's first element should correspond to the LSB.
// `flip` will reverse the fetched bytes (if true)
template <typename T>
std::vector<BYTE> fetchBytes(const T& input, int num, const bool& flip)
{
    std::vector<BYTE> data;
    if (num <= int(sizeof(input))) // available bytes exceed or match requested bytes
    {
        const char* bytePtr = static_cast<const char*>(static_cast<const void*>(&input));
        for(int i = 0; i < num; i++)
        {
            data.push_back(bytePtr[num-1 - i]);
        }
    }
    else                           // too many bytes requested: over-requested bytes are zeroed
    {
        std::cout << "WARNING: Requested " << num << " bytes but only have " << sizeof(input) << '\n';
        const char* bytePtr = static_cast<const char*>(static_cast<const void*>(&input));
        for(int i = 0; i < num; i++)
        {
            if (i < int(sizeof(input)))
                data.push_back(bytePtr[int(sizeof(input))-1 - i]);
            else
                data.push_back(0x00);
        }
    }

    // reverse string bit order (if flip high)
    if (flip && num <= int(sizeof(input)))
        std::reverse(data.begin(), data.end());
    else if (flip && num > int(sizeof(input)))
        std::reverse(data.begin(), std::next(data.begin(), int(sizeof(input))));

    return data;
}

template <typename T>
bool Message::setField(const std::string& dataName, const T& input) // linker does not want this to be defined anywhere else...
{
    if (!this->isEditable()) {std::cout << "ERROR: Message field '" <<  dataName << "' not editable..." << '\n'; return false;}

    /* find the relevant MessageField */
    MessageField *toModify = this->findMessageField(dataName); 
    if (!toModify) {return false;}

    /* modify the data */
    std::vector<BYTE> newBytes;

    // `string` and `bytes` types have to be flipped to match correct bit ordering
    if (toModify->getTypeEnum() == MessageField::string || toModify->getTypeEnum() == MessageField::bytes) newBytes = fetchBytes(input, toModify->getSize(), true);
    else newBytes = fetchBytes(input, toModify->getSize(), false);

    toModify->setData(newBytes);
    return true;
}

#endif // MSG_H

