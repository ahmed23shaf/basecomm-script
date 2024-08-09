#include "log.h"

using namespace el;

uint64_t initTime;

uint64_t timeSinceEpoch()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());
}

std::string getPaddedTimestamp()
{
    uint64_t timeDiff = timeSinceEpoch() - initTime;
    
    if (timeDiff >= MAX_TIMESTAMP)  initTime = timeSinceEpoch(); // reset the timestamps if need be

    std::ostringstream oss;
    oss << std::setw(5) << std::setfill('0') << timeDiff;
    
    return oss.str();
}

std::string bufferToString(uint8_t* buffer, uint8_t& size)
{
    std::ostringstream oss;

    for (uint8_t i = 0; i < size; ++i) 
    {
        oss << std::uppercase
            << std::setw(2)
            << std::setfill('0')
            << std::hex
            << static_cast<int>(buffer[i]);
    }

    return oss.str();
}

void initLogger()
{
    el::Configurations config;
    config.setToDefault();

    // Redirect logger to terminal/file
    if (LOG_TO_FILE) config.set(el::Level::Info, ConfigurationType::ToFile, "true");
    else             config.set(el::Level::Info, ConfigurationType::ToFile, "false");

    if (LOG_TO_TERMINAL) config.set(Level::Info, ConfigurationType::ToStandardOutput, "true");
    else                 config.set(Level::Info, ConfigurationType::ToStandardOutput, "false");

    // Logs directory
    config.set(Level::Info, ConfigurationType::Filename, LOG_PATH);

    // Max size of logs
    int maxBytes = MAX_LOG_SIZE*1024*1024; // ([MiB] * 1024 [KiB/MiB]) * 1024 [bytes/KiB]
    config.set(Level::Info, ConfigurationType::MaxLogFileSize, std::to_string(maxBytes));

    // Flush three log lines max
    el::Loggers::reconfigureAllLoggers(ConfigurationType::LogFlushThreshold, "3");

    // Format
    config.set(el::Level::Info, el::ConfigurationType::Format, "%msg");
    el::Loggers::reconfigureLogger("default", config);

    // Rotating stuff
    el::Loggers::addFlag(el::LoggingFlag::StrictLogFileSizeCheck);
    el::Helpers::installPreRollOutCallback(PreRolloutCallback);

    // Timestamp 
    initTime = timeSinceEpoch();
}

void PreRolloutCallback(const char* fullPath, std::size_t s)
{
    std::string oldFilepath(fullPath);
    std::string pathNoExtension = oldFilepath.substr(0, oldFilepath.find_last_of('.'));

    static int version = MAX_LOGS-1;    // Older logs correspond to greater versions
    
    // Rotate case: max number of log files have been reached
    if (version == 0)
    {
        // Shift previous logs up one.
        // example for SIZE=4
        // .log -> .log.1, .log.1 -> .log.2 -> .log.3, .log.3 -> SHIFTED OUT/OVERWRITTEN
        for (int i = MAX_LOGS-1; i >= 0; i--)
        {
            std::string oldName = pathNoExtension + ".log." + std::to_string(i);
            std::string newName = pathNoExtension + ".log." + std::to_string(i+1);

            // Delete file case
            if (i == MAX_LOGS-1) {remove(oldName.c_str()); continue;}

            // .log -> .log.1 case
            if (i == 0)
            {
                oldName = pathNoExtension + ".log";
                rename(oldName.c_str(), newName.c_str());
                return;
            }

            rename(oldName.c_str(), newName.c_str());
        }
    }

    std::string newFilePath = pathNoExtension + ".log." + std::to_string(version);
    version--;
    rename(fullPath, newFilePath.c_str());
}

void logMessage(comm_error status, DIRECTION dir,  uint8_t* buffer, uint8_t& size)
{
    if (status != NONE)
    {
        switch (status)
        {
            case BAD_CHECKSUM:
                LOG(INFO) << getPaddedTimestamp() << "ms " << "ERROR_BAD_CHECKSUM" << " 0x" << bufferToString(buffer, size);
                break;
            case TIMEOUT:
                LOG(INFO) << getPaddedTimestamp() << "ms " << "ERROR_TIMEOUT";
                break;
            case EMPTY_READ:
                LOG(INFO) << getPaddedTimestamp() << "ms " << "ERROR_MISSING_PACKET";
                break;
        }
        return;
    }

    std::string inOut = (dir == Out) ? "OUT":"IN "; 
    LOG(INFO) << getPaddedTimestamp() << "ms " << inOut << " 0x" << bufferToString(buffer, size);
} 