#include "xml_handler.h"
using namespace pugi;

bool loadDocument(std::string& fileLocation, MessageTable& tableIn)
{
    std::string xmlDirectory = "src/xml/" + fileLocation;   // relative to the exectuable file location

    // Loading file
    xml_document doc;
    if (!doc.load_file(xmlDirectory.c_str())) {std::cout << "ERROR: '" << xmlDirectory << "' failed to load!\n"; return false;}

    for (xml_node command : doc.child("NewDataSet").children("Commands"))
    {
        std::string tag = std::string(command.child("SearchTag").child_value());
        std::transform(tag.begin(), tag.end(), tag.begin(), ::toupper);
        tableIn.addMessage(tag, new Message(command));
    }
    return true;
}