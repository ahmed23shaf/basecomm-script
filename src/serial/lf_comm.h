#ifndef LF_COMM_H
#define LF_COMM_H

#include "../parser/xml_handler.h"
#include "../logger/log.h"
#include "../serial/comm_errors.h"

#include <stdint.h>
#include <regex>
#include <unistd.h>

#define TIMEOUT_MS	 1000		// Message timeout (in ms)
#define BAUD_RATE 	 28800

#define TARGET_IDX	 0
#define PACKLEN_IDX  1
#define SOURCE_IDX 	 3
#define MSG_ID_IDX	 4
#define DATA_IDX	 5

enum comm_mode {
	WRITING,
	WAITING_FOR_MARK,
	READING,
	DONE
};

/*
 * This structure defines parameters and buffers for serial communication.
 * It includes fields for configuration settings, circular data buffers, 
 * state tracking, and error handling.
 */
typedef struct comm_t {
	uint8_t verbose;			// -v flag; if 1, records logs
	uint8_t outBuffer[0x100];	// 256 bytes
	uint8_t inBuffer[0x500];  	// 1280 bytes
	uint16_t head;				// index pointing to one past the last occupied (ex. buffer = [0, 1, 2, ..]; head = 3)
	uint8_t response;			// indicates if a command sent has a corresponding RX message
	uint32_t timeout;
	uint32_t timeoutDuration; 	// packet timeout in seconds
	enum comm_mode mode;
	uint64_t fileDescriptor;
	enum comm_error errorState;
} comm_t;

extern comm_t serialComm;

void printBuffer(uint8_t* buffer, uint8_t& size);

bool validateChecksum();

/********** PLATFORM-SPECIFIC FUNCTIONS **********/
bool isValidComPort(const std::string& input);

// Takes in the inputs passed and sets the appropriate variables
// Performs error checking
bool processInput(int& argc, char **&argv);

// @return true if successful
bool initComm(int& argc, char **&argv);

/*
 * Before calling this function, the OUT comm buffer is expected to be ready.
 * This means that the entire message needs to be on the buffer starting at idx=0.
 * 
 * Example of a valid buffer:
 * {0x11, 0x06, 0x00, 0xF0, 0x06, 0xF3, ...}
 * where 0x11 corresponds to idx=0. 
 * 
 * `serialComm.head` SHOULD BE AT IDX=0. (Used to index bytes to send at a time)
 */
void commWrite();

/*
 * Will be called after commWrite() has taken place to handle response packets
 * Expects `serialComm.head` to be reset to 0 prior to reading a response
 */
void commRead();

/*************************************************/

// Used to transition between reading/writing
void commFSM();

// @return true on success
bool sendPacket(Message *toBeSent);

// By the time this function is called,
// inBuffer should already be populated
void handleResponse();

#endif // LF_COMM_H