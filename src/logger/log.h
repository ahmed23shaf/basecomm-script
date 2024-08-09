#ifndef LOG_H
#define LOG_H

#define LOG_TO_TERMINAL  false                                      // prints logs to terminal if true
#define LOG_TO_FILE      true                    
#define LOG_PATH         "logs/data.%datetime{%M%d_%H%m%s}.log"     // relative to the executable file
#define MAX_LOGS         6                                          // maximum log files before logger starts overwriting
#define MAX_LOG_SIZE     4                                          // in MiB (basically MB)
#define MAX_LOG_TIME     180                                        // in minutes; time to manually rotate logs (unimplemented)

#include "../lib/easylogging/easylogging++.h"

#include <chrono>
#include <iomanip>
#include "../serial/comm_errors.h"
#define MAX_TIMESTAMP   uint64_t(99999)

enum DIRECTION
{
    Out,
    In
};

extern uint64_t initTime;   // time since epoch of the when program began

uint64_t timeSinceEpoch();  // in ms

// Time difference between current and program init
// formmatted as a zero-padded 5 digit number
std::string getPaddedTimestamp();

// Takes in a buffer (typically outBuffer or inBuffer) as well as the size 
// to return a printable hex string
std::string bufferToString(uint8_t* buffer, uint8_t& size);

/*
 * Uses the defined macros to setup a rotating logger.
 * Changes the formatting of EasyLogging++ to conform to Sniffer clicker standards
 */
void initLogger();

// Executes when the top-most (or most recent) .log file gets full.
// Log rotation logic implemented here
void PreRolloutCallback(const char* fullPath, std::size_t size);

/*
 * Takes in information about a message (or error) and logs it universally
 * The format of Sniffer logs is explained below:
 * 
 * SSSSSms IN  0x<DATA_BUFFER>
 *          OR
 * SSSSSms OUT 0x<DATA_BUFFER>
 * 
 * SSSSS is the zero-padded timestamp (in ms) which rolls over when it reaches 99999
 */
void logMessage(comm_error status, DIRECTION dir,  uint8_t* buffer, uint8_t& size);

#endif // LOG_H