#ifndef MSG_TABLE_H
#define MSG_TABLE_H

#include <map>

#include "msg.h"

/**
 * @brief The MessageTable class (Look Up Table)
 */

class MessageTable
{
    public:
        // Setters
        void addMessage(std::string& searchTag, Message* msg); //recursive
        void removeMessage(std::string& searchTag);
        void updateMessage(std::string& searchTag, Message* msg);

        // Getters
        std::map<std::string, Message*> getMsgTable() {return m_msgTable;}
        Message* findMessageByTag(std::string& searchTag);
        Message* findMessage(const std::string& name);
        std::string generateSearchTag(std::string &senderID, std::string &targetID, std::string &cmdID, std::string &length);
        std::vector<std::string> getMessageNames();
        std::vector<std::string> getMessageTags();
        bool empty();

        void printTable();
    private:
        std::map<std::string, Message*> m_msgTable; // maps searchTags (ex. "11:F0:0D") to Message structures (ex. Version_Command (LS))
};

extern MessageTable table;

#endif // MSG_TABLE_H
