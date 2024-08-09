#include "src/serial/lf_comm.h"
using namespace std;
INITIALIZE_EASYLOGGINGPP

MessageTable table;

typedef uint8_t                 BITFIELD;
typedef uint8_t                 BYTE;
typedef uint16_t                WORD;
typedef std::string             STRING;
typedef uint32_t                LONG_INT;
typedef int16_t                 SIGNED_WORD;
typedef uint8_t*                BYTES;
typedef std::vector<uint16_t>   WORDS;
typedef uint64_t                LONG_LONG;

/***************** XML *****************/
STRING xmlFile = "dtCommandsTMEV.xml";
/***************************************/

int main (int argc, char **argv)
{
    /* IGNORE BELOW */
    if (!loadDocument(xmlFile, table)) return 1;
    if (!initComm(argc, argv))         return 1;
    initLogger();
    /* IGNORE ABOVE */

    
}