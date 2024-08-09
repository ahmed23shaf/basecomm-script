#include "lf_comm.h"

#if defined(_WIN32) || defined(_WIN64)
    #include "windows_comm.cpp"
#elif defined(__linux__)
    #include "linux_comm.cpp"
#else
    #error "Unsupported Platform"
#endif

comm_t serialComm = {0};

void printBuffer(uint8_t* buffer, uint8_t& size) 
{
    for (uint8_t i = 0; i < size; ++i)
        std::cout << std::uppercase 
                  << std::setw(2) 
                  << std::setfill('0') 
                  << std::hex 
                  << static_cast<int>(buffer[i]);
                //   << ", ";
    std::cout << '\n' << std::dec;
}

bool validateChecksum()
{
    // Recall the checksum is computed as the Two's Complement (or negation)
    // of the sum of all bytes within a message. Adding all the bytes AND the
    // checksum byte should yield 0 for a valid message using the wrap around (modulo).
    // behavior of unsigned integers.

    uint8_t checksum = 0;
	for (uint8_t i = 0; i < serialComm.inBuffer[PACKLEN_IDX]; i++)
        checksum += serialComm.inBuffer[i];
    
    return checksum == 0;
}

void commFSM()
{
    switch (serialComm.mode) 
    {
	    case WRITING:
		    commWrite();
		    break;
	    case WAITING_FOR_MARK:  // idk why this state exists but ok
	    case READING:
		    commRead();
		    break;
	    default:
		    break;
	}
}

bool sendPacket(Message *toBeSent)
{
    // Expect a response from EVERY message
    // NOTE that BOOT commands may not have a response, need to add functionality for this later
    serialComm.response = 1;

    std::vector<uint8_t> msgBytes = toBeSent->getMessageBuffer();
    for (size_t i = 0; i < msgBytes.size(); i++)
    {
        serialComm.outBuffer[i] = msgBytes[i];
    }

    // Send packet until done
    serialComm.mode = WRITING;
    serialComm.head = 0;
    serialComm.errorState = NONE;

    while (serialComm.mode != DONE)
    {
        commFSM();
        usleep(100000);

        if (serialComm.errorState != NONE) return false;
    }

    return true;
}

void handleResponse()
{
    std::string incomingTag;
    std::stringstream ss;

    // uint8_t --> hex string
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2)
    << static_cast<int>(serialComm.inBuffer[TARGET_IDX]) << ":"
    << std::setw(2) << static_cast<int>(serialComm.inBuffer[SOURCE_IDX]) << ":"
    << std::setw(2) << static_cast<int>(serialComm.inBuffer[MSG_ID_IDX]);

    incomingTag = ss.str();
    
    Message *toUpdate = table.findMessageByTag(incomingTag);

    // Try appending the packet length and looking (ex. Push Report)
    if (toUpdate == nullptr)
    {
        std::stringstream ss1;
        ss1 << std::hex << std::uppercase << std::setfill('0') << std::setw(2)
        << static_cast<int>(serialComm.inBuffer[TARGET_IDX]) << ":"
        << std::setw(2) << static_cast<int>(serialComm.inBuffer[SOURCE_IDX]) << ":"
        << std::setw(2) << static_cast<int>(serialComm.inBuffer[MSG_ID_IDX]) << ":"
        << std::setw(2) << static_cast<int>(serialComm.inBuffer[PACKLEN_IDX]);
        incomingTag = ss1.str();
        toUpdate = table.findMessageByTag(incomingTag);
    }

    int dataWidth = serialComm.inBuffer[PACKLEN_IDX] - MIN_PACK_LEN;
    toUpdate->setDataBuffer(std::vector<BYTE>(serialComm.inBuffer + DATA_IDX, serialComm.inBuffer + DATA_IDX + dataWidth));
}