#include "lf_comm.h"
#include <windows.h>

std::string usbFile = "\\\\.\\";

bool isValidComPort(const std::string& input) 
{
    return std::regex_match(input, std::regex("^COM\\d{1,3}$"));
}

bool processInput(int& argc, char **&argv)
{
    // Not enough arguments case
    if (argc <= 1) {std::cout << "ERROR: No COM port specified.\nTry something like this:\n.\\main.exe COM3"; return false;}

    // Only COM specified
    if (argc == 2)
    {
        if (!isValidComPort(std::string(argv[1]))) {std::cout << "ERROR: Invalid COM port specified."; return false;}
        usbFile += std::string(argv[1]);
        return true;
    }

    // COM and Verbose flag 
    if (argc == 3 && (!strcmp(argv[2], "-v") || !strcmp(argv[2], "-V") || !strcmp(argv[2], "--verbose")))
    {
        if (!isValidComPort(std::string(argv[1]))) {std::cout << "ERROR: Invalid COM port specified."; return false;}
        usbFile += std::string(argv[1]);

        serialComm.verbose = 1;
        return true;
    }

    std::cout << "ERROR: Too many arguments.\nTry something like this:\n.\\main.exe COM3";
    return false;
}

bool initComm(int& argc, char **&argv)
{
        HANDLE hCom;
        DCB dcb = {0};
        COMMTIMEOUTS timeouts = {0};

        if (!processInput(argc, argv)) return false;

        // Open Serial Port
        hCom = CreateFile(usbFile.c_str(),
                        GENERIC_READ | GENERIC_WRITE,
                        0,      //  must be opened with exclusive-access
                        NULL,   //  default security attributes
                        OPEN_EXISTING, //  must use OPEN_EXISTING
                        0,      //  not overlapped I/O
                        NULL ); //  hTemplate must be NULL for comm devices

        if (hCom == INVALID_HANDLE_VALUE) {std::cout << "CreateFile() failed with error " << GetLastError() << '\n'; return false;}

        // Configure Serial Port
        dcb.DCBlength = sizeof(DCB);
        dcb.BaudRate = BAUD_RATE;
        dcb.fBinary = 1;
        dcb.fParity = 1;
        dcb.ByteSize = 8;
        dcb.Parity = MARKPARITY;    // send Dest ID with MARK then change to SPACE
        dcb.StopBits = ONESTOPBIT;
        SetCommState(hCom, &dcb);

        // Configure Timeouts -- MAXDWORD for ReadInterval defaults to the TIMEOUT in ReadTotalTimeoutConstant
        timeouts.ReadIntervalTimeout = MAXDWORD;

        timeouts.ReadTotalTimeoutConstant = TIMEOUT_MS;
        timeouts.ReadTotalTimeoutMultiplier = 0;

        timeouts.WriteTotalTimeoutConstant = 0;
        timeouts.WriteTotalTimeoutMultiplier = 0;
        SetCommTimeouts(hCom, &timeouts);

        serialComm.fileDescriptor = (uint64_t) hCom;
        return true;
}

void commWrite()
{
    HANDLE hCom = (HANDLE)(serialComm.fileDescriptor);
    BOOL status;
    DCB dcb;

    unsigned long bytesToBeWritten = (serialComm.head) ? (serialComm.outBuffer[PACKLEN_IDX] - serialComm.head):(1); // Initially write first byte with MARK, then write all other bytes
    unsigned long bytesWritten;

    status = WriteFile(hCom, &serialComm.outBuffer[serialComm.head], bytesToBeWritten, &bytesWritten, NULL);

    // Error handling
    if (status == -1) 
    {
		std::cout << "ERROR: bad write in interrupt\nerrorno: " << strerror(errno) << '\n';
		exit(1);
	}
    else if (bytesWritten == 0)
    {
        std::cout << "ERROR: commWrite() wrote 0 bytes\n";
		exit(1);
    }

    serialComm.head += bytesWritten;    // Advance head index pointer

    // After writing first byte
    if (serialComm.head == 1)
    {
        // Log the data
        if (serialComm.verbose) logMessage(serialComm.errorState, Out, serialComm.outBuffer, serialComm.outBuffer[PACKLEN_IDX]);

        usleep(1000);   // Delay to avoid changing parity on the previously written byte.

        GetCommState(hCom, &dcb);
        dcb.Parity = SPACEPARITY;
        SetCommState(hCom, &dcb);
    }
    // After writing all bytes
    else if (serialComm.head >= serialComm.outBuffer[PACKLEN_IDX])
    {
        // Update flags and prepare to read
        if (serialComm.response) serialComm.mode = WAITING_FOR_MARK;
        else                     serialComm.mode = DONE;

        // reset head 'pointer'
        serialComm.head = 0;
    }
}

void commRead()
{
    HANDLE hCom = (HANDLE)(serialComm.fileDescriptor);
    BOOL status;
    DCB dcb;

    unsigned long bytesRead;

    // Read and append incoming data to end of the input buffer
    uint64_t timePreRead = timeSinceEpoch();
	status = ReadFile(hCom, &serialComm.inBuffer[serialComm.head], sizeof(serialComm.inBuffer) - serialComm.head, &bytesRead, NULL);
    uint64_t timeAfter =   timeSinceEpoch();
    // Error handling
    if (!status)
    {
        if (GetLastError() == ERROR_TIMEOUT)
        {
            serialComm.errorState = TIMEOUT;
            if (serialComm.verbose) logMessage(serialComm.errorState, In, serialComm.inBuffer, serialComm.inBuffer[PACKLEN_IDX]);
        }
        else
        {
            std::cout << "ERROR: bad read \nerrorno: " << strerror(errno) << '\n';
            exit(1);
        }
	}
    else if (timeAfter - timePreRead > TIMEOUT_MS)
    {
        serialComm.errorState = TIMEOUT;
        if (serialComm.verbose) logMessage(serialComm.errorState, In, serialComm.inBuffer, serialComm.inBuffer[PACKLEN_IDX]);
    }
    else if (bytesRead == 0)
    {
        serialComm.errorState = EMPTY_READ;
        if (serialComm.verbose) logMessage(serialComm.errorState, In, serialComm.inBuffer, serialComm.inBuffer[PACKLEN_IDX]);
    }
    else
    {
        serialComm.head += bytesRead;

        // After all bytes have been read in
        if (serialComm.head >= MIN_PACK_LEN && serialComm.head >= serialComm.inBuffer[PACKLEN_IDX])
        {
            // Check for a good CS
            if (!validateChecksum())
            {
                std::cout << "ERROR_BAD_CHECKSUM 0x";
                printBuffer(serialComm.inBuffer, serialComm.inBuffer[PACKLEN_IDX]);
                serialComm.errorState = BAD_CHECKSUM;
            }

            // Capture logs
            else if (serialComm.verbose) logMessage(serialComm.errorState, In, serialComm.inBuffer, serialComm.inBuffer[PACKLEN_IDX]);

            serialComm.mode = DONE;
        }
    }

    if (serialComm.errorState != NONE || serialComm.mode == DONE)
    {
        serialComm.mode = DONE;
        serialComm.head = 0;

        GetCommState(hCom, &dcb);
        dcb.Parity = MARKPARITY;
        SetCommState(hCom, &dcb);
    }
}
