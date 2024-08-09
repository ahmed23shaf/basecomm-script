#include "msg_table.h"

// SETTERS ===================================================================================

/**
 * @brief Add a message to the table
 * @param searchTag (ex. F0:11:89)
 * @param msg       (ex. {sibstat0,bitfield,1,bit 0 = flyback on,...})
 * @return success
 */
void MessageTable::addMessage(std::string& searchTag, Message* msg)
{
    if (findMessageByTag(searchTag) != nullptr)
    {
        std::cout << "Message " << searchTag << " already exists, overwriting!";
        m_msgTable[searchTag] = msg;
        return;
    }
    m_msgTable.insert({searchTag, msg});
}

void MessageTable::removeMessage(std::string &searchTag)
{
    m_msgTable.erase(m_msgTable.find(searchTag));
}

void MessageTable::updateMessage(std::string &searchTag, Message *msg)
{
    removeMessage(searchTag);
    addMessage(searchTag, msg);
}


// GETTERS =============================================================================

/**
 * @brief MessageTable::findMessageByTag
 * Will search for a message in the look up table and return message details
 * @param searchTag
 * @return Message pointer, or null on failure
 */
Message* MessageTable::findMessageByTag(std::string& searchTag)
{
    if (m_msgTable.find(searchTag) != m_msgTable.end())
    {
        return m_msgTable.at(searchTag);
    }
    else
    {
        return nullptr;
    }
}

/**
 * @brief MessageTable::findMessage
 * Will search for message by name, this is slower than search tag
 * Case insensitive
 * @param name
 * @return Message pointer, or null on failure
 */
Message *MessageTable::findMessage(const std::string &name)
{
    std::string lowerCaseName = name;
    std::transform(lowerCaseName.begin(), lowerCaseName.end(), lowerCaseName.begin(), ::tolower);
    for (std::map<std::string, Message*>::iterator itr = m_msgTable.begin();
         itr != m_msgTable.end();
         itr++)
    {
        std::string itrName = itr->second->getMsgName();
        std::transform(itrName.begin(), itrName.end(), itrName.begin(), ::tolower);
        if (itrName == lowerCaseName)
            return itr->second;
    }
    std::cout << "ERROR: Message '" <<  name << "' not found..." << '\n';
    exit(1);
    return nullptr;
}

/**
 * @brief MessageTable::generateSearchTag
 * Creates search tag string from parameters
 * @param senderID, targetID, cmdID, length
 * @return search tag as formatted string
 */
std::string MessageTable::generateSearchTag(std::string& senderID, std::string& targetID, std::string& cmdID, std::string& length)
{
    std::string searchTag = targetID + ":" + senderID + ":" + cmdID;
    if (cmdID == "1E")
    {
        if (senderID == "F0" || senderID == "BC") // from console/tester?
            searchTag += ":" + length;
    }
    else if (cmdID == "92")
    {
        if (senderID == "10")    // from base?
            searchTag += ":" + length;
    }
    return searchTag;
}

std::vector<std::string> MessageTable::getMessageNames()
{
    std::vector<std::string> names;
    for (std::map<std::string, Message*>::iterator itr = m_msgTable.begin();
         itr != m_msgTable.end();
         itr++)
    {
        names.push_back(itr->second->getMsgName());
    }
    return names;
}

std::vector<std::string> MessageTable::getMessageTags()
{
    std::vector<std::string> tags;
    for (std::map<std::string, Message*>::iterator itr = m_msgTable.begin();
         itr != m_msgTable.end();
         itr++)
    {
        tags.push_back(itr->first);
    }
    return tags;
}

/**
 * @brief MessageTable::empty
 * @return True if table is empty
 */
bool MessageTable::empty()
{
    return m_msgTable.empty();
}

void MessageTable::printTable()
{
    for (auto const& pair : m_msgTable)
    {
        std::cout << "\n***************************************" << '\n';
        std::cout << "PAIR: " << pair.first << " :|: " << pair.second->getSearchTag() << '\n';
        std::cout << "***************************************" << '\n';

        pair.second->printMsg();
    }
}