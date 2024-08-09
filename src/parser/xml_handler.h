#ifndef XML_HANDLER_H
#define XML_HANDLER_H

#include "msg_table.h"

/**
 * @brief Loads a document from a specified file location into a MessageTable.
 * 
 * @param fileLocation The path to the file to be loaded.
 * @param tableIn      Reference to a MessageTable where the loaded messages will be stored.
 * 
 * @return true if sucessfully loaded into `tableIn`
 */
bool loadDocument(std::string& fileLocation, MessageTable& tableIn);

#endif // XML_HANDLER_H