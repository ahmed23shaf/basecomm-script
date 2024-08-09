#include "msg.h"
#include "../serial/lf_comm.h"

Message::Message(xml_node commandNode)
{
    m_SearchTag     = commandNode.child("SearchTag").child_value();     // node does not have a value, but instead has a child of type `node_pcdata` with value
    m_TargetID      = commandNode.child("TargetID").child_value();
    m_SenderID      = commandNode.child("SenderID").child_value();
    m_MsgValue      = commandNode.child("MsgValue").child_value();
    m_MsgName       = commandNode.child("MsgName").child_value();

    // Uppercase all hex fields
    std::transform(m_SearchTag.begin(), m_SearchTag.end(), m_SearchTag.begin(), ::toupper);
    std::transform(m_TargetID.begin(), m_TargetID.end(), m_TargetID.begin(), ::toupper);
    std::transform(m_SenderID.begin(), m_SenderID.end(), m_SenderID.begin(), ::toupper);
    std::transform(m_MsgValue.begin(), m_MsgValue.end(), m_MsgValue.begin(), ::toupper);    

    // Converting packet length to base-10 int [HEX->INT]
    std::istringstream converter(commandNode.child("PackLen").child_value());   
    converter >> std::hex >> m_PackLen;

    m_DuplicateCmd  = commandNode.child("DuplicateCmd").child_value();

    std::string isBootMode = commandNode.child("IsBootModeCmd").child_value();
    std::transform(isBootMode.begin(), isBootMode.end(), isBootMode.begin(), ::tolower);
    m_IsBootModeCmd = (isBootMode == "true");
    
    m_CmdNotes      = commandNode.child("CmdNotes").child_value();

    // Data
    std::vector<std::string> DataNames   = split(commandNode.child("DataNames").child_value(),',');
    std::vector<std::string> DataTypes   = split(commandNode.child("DataTypes").child_value(),',');
    std::vector<std::string> DataSizes   = split(commandNode.child("DataSizes").child_value(),',');
    std::vector<std::string> DataDetails = split(commandNode.child("DataDetails").child_value(),',');
    std::vector<std::string> DefaultData = split(commandNode.child("DefaultData").child_value(), ','); // if any

    int j = 0; // DefaultData iterator

    for (size_t i = 0; i < DataNames.size(); ++i)
    {
        std::vector<std::string> defaultDataPerField;
        if (!DefaultData.empty())
        {
            int dataSize = std::stoi(DataSizes.size() > i ? DataSizes[i] : std::string("0"));
            int old_j = j;
            while (j < int(DefaultData.size()) && j < old_j+dataSize)    // append to defaultData
            {
                defaultDataPerField.push_back(DefaultData[j]);
                j++;
            }
        }

        data_format.push_back(new MessageField(  DataNames.size() > i      ?   DataNames[i]    : std::string(),
                                                 DataTypes.size() > i      ?   DataTypes[i]    : std::string(),
                                                 DataSizes.size() > i      ?   DataSizes[i]    : std::string(),
                                                 DataDetails.size() > i    ?   DataDetails[i]  : std::string(),
                                                 defaultDataPerField));
    }
}

comm_error Message::sendMessage()
{
    if (!this->isOutgoing())
    {
        std::cout << getMsgName() << " is NOT an outgoing message...";
        return INVALID_MSG;
    }
    if (!sendPacket(this)) 
    {
        std::cout << "ERROR: sendPacket() failed with error: " << serialComm.errorState << '\n';
        return serialComm.errorState;
    }

    handleResponse();
    return NONE;
}

std::vector<BYTE> Message::getHeader()
{// TARGET ID, LENGTH, SEQUENCE, SENDER ID, MSG CODE
    std::vector<BYTE> header;
    int hexResult;

    // TARGET ID
    std::istringstream(m_TargetID) >> std::hex >> hexResult;
    header.push_back(static_cast<BYTE>(hexResult));

    // LENGTH
    header.push_back(static_cast<BYTE>(m_PackLen));

    // SEQUENCE (UNIMPLEMENTED)
    header.push_back(BYTE(0x00));

    // SENDER ID
    std::istringstream(m_SenderID) >> std::hex >> hexResult;
    header.push_back(static_cast<BYTE>(hexResult));

    // MSG CODE
    std::istringstream(m_MsgValue) >> std::hex >> hexResult;
    header.push_back(static_cast<BYTE>(hexResult));

    return header;
}

std::vector<BYTE> Message::getDataBuffer()
{
    std::vector<BYTE> buffer;
    for (MessageField*& piece : data_format)
    {
        std::vector<BYTE> piece_data = piece->getData();
        for (BYTE& byte : piece_data)
            buffer.push_back(byte);
    }
    return buffer;
}

/* Algorithm for message checksum:
*  Two's complement of the sum of the header and data bytes.
*
*  In other words, take the sum of all bytes in a packet (except the CS) and negate it.
*/
BYTE Message::getChecksum()
{
    std::vector<BYTE> messageBufferNoCS;

    std::vector<BYTE> headerBytes = this->getHeader();
    std::vector<BYTE> dataBytes = this->getDataBuffer();

    messageBufferNoCS.insert(messageBufferNoCS.end(), headerBytes.begin(), headerBytes.end());
    messageBufferNoCS.insert(messageBufferNoCS.end(), dataBytes.begin(), dataBytes.end());

    BYTE checksum = 0;

	for (const BYTE& b : messageBufferNoCS)
        checksum += b;

	return -1*checksum;
}

std::vector<BYTE> Message::getMessageBuffer()
{
    std::vector<BYTE> message;

    // Header bytes
    std::vector<BYTE> headerBytes = getHeader();
    message.insert(message.end(), headerBytes.begin(), headerBytes.end());

    // Data buffer
    std::vector<BYTE> dataBytes = getDataBuffer();
    message.insert(message.end(), dataBytes.begin(), dataBytes.end());

    // CS
    message.push_back(this->getChecksum());

    return message;
}

MessageField* Message::findMessageField(std::string fieldName)
{
    std::transform(fieldName.begin(), fieldName.end(), fieldName.begin(), ::toupper);
    std::string currentName;
    
    MessageField *desired = nullptr;
    for (MessageField*& entry : data_format)
    {
        currentName = entry->getName();
        std::transform(currentName.begin(), currentName.end(), currentName.begin(), ::toupper);
        if (currentName == fieldName) {desired = entry; break;} 
    }

    if (desired == nullptr) {std::cout << "ERROR: Message field '" << fieldName << "' not found..." << '\n';}
    return desired;
}

std::string Message::getMsgDirectionStr()
{
    return deviceIDtoStr(m_SenderID) + " --> " + deviceIDtoStr(m_TargetID);
}

void Message::setSearchTag (std::string SearchTag)
{
    if (checkSearchTag(SearchTag))
    {
        m_SearchTag = SearchTag;
        std::transform(m_SearchTag.begin(), m_SearchTag.end(), m_SearchTag.begin(), ::toupper);
        updateSearchTag(fromSearchTag);
    }
}

void Message::setTargetID (std::string TargetID)
{
    if (checkByteInput(TargetID))
    {
        m_TargetID  = TargetID;
        std::transform(m_TargetID.begin(), m_TargetID.end(), m_TargetID.begin(), ::toupper);
        updateSearchTag(fromID);
    }
}

void Message::setSenderID (std::string SenderID)
{
    if (checkByteInput(SenderID))
    {
        m_SenderID  = SenderID;
        std::transform(m_SenderID.begin(), m_SenderID.end(), m_SenderID.begin(), ::toupper);
        updateSearchTag(fromID);
    }
}

void Message::setMsgValue (std::string MsgValue)
{
    if (checkByteInput(MsgValue))
    {
        m_MsgValue = MsgValue;
        std::transform(m_MsgValue.begin(), m_MsgValue.end(), m_MsgValue.begin(), ::toupper);
        updateSearchTag(fromID);
    }
}

void Message::updateSearchTag(searchTagUpdate type)
{
    switch (type)
    {
        case fromSearchTag:
        {
            std::vector<std::string> ids = split(m_SearchTag, ':');
            if (ids.size() == 3 || ids.size() == 4)
            {
                m_TargetID = ids[0];
                m_SenderID = ids[1];
                m_MsgValue = ids[2];
                if (ids.size() == 4)
                    m_PackLen = std::stoi(ids[3]);
            }
        }
            break;
        case fromID:
        {
            std::string newSearchTag;
            newSearchTag += m_TargetID;
            newSearchTag += ":" + m_SenderID;
            newSearchTag += ":" + m_MsgValue;
            if (split(m_SearchTag, ':').size() == 4)
            {
                std::stringstream ss;
                ss << std::hex << m_PackLen;
                std::string hex = ss.str();
                std::transform(hex.begin(), hex.end(), hex.begin(), ::toupper);
            }
            m_SearchTag = newSearchTag;
        }
            break;
    }
}

std::string Message::deviceIDtoStr(const std::string& id)
{
    if (id == "10") return std::string("PTC_HS");
    if (id == "11") return std::string("PTC_LS");
    if (id == "18") return std::string("pm_MDB");   //powermill
    if (id == "19") return std::string("pm_SIB");   //powermill
    if (id == "20") return std::string("INCLINE");
    if (id == "40") return std::string("EEPROM");
    if (id == "80") return std::string("PNC");
    if (id == "99") return std::string("99");
    if (id == "F0") return std::string("M4");
    if (id == "BC") return std::string("TE");
    return std::string("n/a");
}

bool Message::checkSearchTag(std::string& st)
{
    std::vector<std::string> ids = split(st, ':');
    if (ids.size() == 3 || ids.size() == 4)
    {
        bool ok = 1;
        for (std::string id : ids)
        {
            ok &= (id.length() == 2);

            // checking valid hex input
            if (!std::all_of(id.begin(), id.end(), ::isxdigit)) 
            {
                std::cerr << "Bad input: " << st << '\n';
                return false;
            }

            std::istringstream hexToInt(id);
            uint hex;
            hexToInt >> std::hex >> hex;
            ok &= (hex < 256 && hex >= 0);
        }
        if (ok)
        {
            return true;
        }
    }
    std::cerr << "Bad search tag: " << st << '\n';
    return false;
}

bool Message::checkByteInput(std::string& str)
{
    bool ok = 1;

    // checking valid hex input
    if (!std::all_of(str.begin(), str.end(), ::isxdigit)) 
    {
        std::cerr << "Bad input: " << str << '\n';
        return false;
    }

    std::istringstream hexToInt(str);
    uint hex;
    hexToInt >> std::hex >> hex;
    ok &= (hex < 256 && hex >= 0);
    if (!ok)
    {
        std::cerr << "Bad input: " << str << '\n';
    }
    return ok;
}

void Message::printMsg()
{
    std::cout << m_SearchTag << " | " << m_TargetID << m_SenderID << m_MsgValue << '\n';
    std::cout << m_MsgName << " PackLen: " << m_PackLen << '\n';

    for (MessageField* piece : data_format)
    {
        piece->printMsgField();
    }
}

void Message::setDataBuffer(const std::vector<BYTE>& newBuffer)
{
    // Size mismatch: copy over the first bytes from newBuffer
    if (int(newBuffer.size()) != (m_PackLen - MIN_PACK_LEN)) {return;}

    int byteIdx = 0;
    for (MessageField *& piece : data_format)
    {
        piece->setData(std::vector<BYTE>(newBuffer.begin() + byteIdx, newBuffer.begin() + byteIdx + piece->getSize()));
        byteIdx += piece->getSize();
    }
}

template<typename T>
T stitchIntBytes(std::vector<BYTE> bytes)   // Helper function for stiching int based bytes
{
    T stichedInt;
    std::reverse(bytes.begin(), bytes.end());   // to correct for processor endianness
    std::memcpy(&stichedInt, bytes.data(), sizeof(T));
    return stichedInt;
}

// Template specializations for getField(std::string)
template<> // bitfield byte, byte
BYTE Message::getField<BYTE>(const std::string& dataName)
{
    /* find the field */
    MessageField *toRead = findMessageField(dataName);
    if (!toRead) return BYTE();

    /* stitch the bytes */
    return toRead->getData()[0];
}

template<> // word
uint16_t Message::getField<uint16_t>(const std::string& dataName)
{
    /* find the field */
    MessageField *toRead = findMessageField(dataName);
    if (!toRead) return uint16_t();

    /* stitch the bytes */
    return stitchIntBytes<uint16_t>(toRead->getData());
}

template<> // string
std::string Message::getField<std::string>(const std::string& dataName)
{
    /* find the field */
    MessageField *toRead = findMessageField(dataName);
    if (!toRead) return std::string();

    std::vector<BYTE> localBytes = toRead->getData();
    /* stitch the bytes */
    return std::string(localBytes.begin(), localBytes.end());
}

template<> // long
uint32_t Message::getField<uint32_t>(const std::string& dataName)
{
    /* find the field */
    MessageField *toRead = findMessageField(dataName);
    if (!toRead) return uint32_t();

    /* stitch the bytes */
    return stitchIntBytes<uint32_t>(toRead->getData());
}

template<> // signed word
int16_t Message::getField<int16_t>(const std::string& dataName)
{
    /* find the field */
    MessageField *toRead = findMessageField(dataName);
    if (!toRead) return int16_t();

    /* stitch the bytes */
    return stitchIntBytes<int16_t>(toRead->getData());
}

template<> // byteS
BYTE* Message::getField<BYTE*>(const std::string& dataName)
{
    /* find the field */
    MessageField *toRead = findMessageField(dataName);
    if (!toRead) return int16_t();

    /* stitch the bytes */
    return toRead->getData().data();
}

template<> // wordS: vector of uint16_t
std::vector<uint16_t> Message::getField<std::vector<uint16_t>>(const std::string& dataName)
{
    /* find the field */
    MessageField *toRead = findMessageField(dataName);
    if (!toRead) return std::vector<uint16_t>();

    /* stitch the bytes */
    std::vector<BYTE> bytes = toRead->getData();
    std::vector<uint16_t> result(toRead->getSize() / 2, 0);
    for (int i = 0; i < (toRead->getSize() / 2); i++)
    {
        size_t byteIdx = i*2;
        std::vector<BYTE> currentTwoBytes = {bytes[byteIdx], bytes[byteIdx+1]};

        result[i] = stitchIntBytes<uint16_t>(currentTwoBytes);
    }
    return result;
}

template<> // long long
uint64_t Message::getField<uint64_t>(const std::string& dataName)
{
    /* find the field */
    MessageField *toRead = findMessageField(dataName);
    if (!toRead) return uint64_t();

    /* stitch the bytes */
    return stitchIntBytes<uint64_t>(toRead->getData());
}