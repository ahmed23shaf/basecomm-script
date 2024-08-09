#include "lf_comm.h"

#include <sys/ioctl.h>
#include <asm/ioctls.h>
#include <asm/termbits.h>
#include <fcntl.h>

std::string usbFile;

bool isValidComPort(const std::string& input) 
{
    return std::regex_match(input, std::regex("^/dev/tty[A-Za-z0-9]*$"));
}

bool processInput(int& argc, char **&argv)
{
    // Not enough arguments case
    if (argc <= 1) {std::cout << "ERROR: No USB device file specified.\nTry something like this:\n./main /dev/ttyUSB0\n"; return false;}

    // Only USB device file specified
    if (argc == 2)
    {
        if (!isValidComPort(std::string(argv[1]))) {std::cout << "ERROR: Invalid USB device file specified.\n"; return false;}
        usbFile = std::string(argv[1]);
        return true;
    }

    // USB device file + verbose flag
    if (argc == 3 && (!strcmp(argv[2], "-v") || !strcmp(argv[2], "-V") || !strcmp(argv[2], "--verbose")))
    {
        if (!isValidComPort(std::string(argv[1]))) {std::cout << "ERROR: Invalid USB device file specified.\n"; return false;}
        usbFile = std::string(argv[1]);

        serialComm.verbose = 1;
        return true;
    }

    std::cout << "ERROR: Too many arguments.\nTry something like this:\n./main.exe /dev/ttyUSB0";
    return false;
}

bool initComm(int& argc, char **&argv)
{
    struct termios2 tio;

    // Ensure correct input
    if (!processInput(argc, argv)) return false;

    // Open Serial port
	serialComm.timeoutDuration = 2;
	serialComm.fileDescriptor  = open(usbFile.c_str(), O_RDWR | O_NOCTTY);

	if (serialComm.fileDescriptor == uint64_t(-1)) {std::cout << "open() failed with error " << strerror(errno) << '\n'; return false;}

    // Configure Serial Port
    ioctl(serialComm.fileDescriptor, TCGETS2, &tio);
	tio.c_cflag &= ~CBAUD;
	tio.c_cflag |= PARENB | CMSPAR | PARODD | BOTHER;
	tio.c_iflag &= ~(IXON | IGNCR | ICRNL | IGNBRK | BRKINT);
	tio.c_iflag |= INPCK | PARMRK;
	tio.c_oflag &= ~OPOST;
	tio.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tio.c_ispeed = BAUD_RATE;
	tio.c_ospeed = BAUD_RATE;
    tio.c_cc[VMIN] = 0; // min number of bytes per read

    // Configure Timeouts
    tio.c_cc[VTIME] = TIMEOUT_MS/100; // TIMEOUT/10 seconds timeout between empty reads

    ioctl(serialComm.fileDescriptor, TCSETS2, &tio);
    return true;
}

void commWrite()
{
    /*
     * Transmit first byte with mark (parity) set or transmit all other bytes together
     * with space (parity) set. 
     */
    int bytesToBeWritten = (serialComm.head) ? (serialComm.outBuffer[PACKLEN_IDX] - serialComm.head):(1);
    int bytesWritten     = write(serialComm.fileDescriptor, &serialComm.outBuffer[serialComm.head], bytesToBeWritten);

    // Error handling
    if (bytesWritten == -1)
    {
        std::cout << "ERROR: bad write in interrupt\nerrorno: " << strerror(errno) << '\n';
		exit(1);
    }
    else if (bytesWritten == 0)
    {
        std::cout << "ERROR: commWrite() wrote 0 bytes\n";
		exit(1);
    }

    serialComm.head += bytesWritten;

    // After writing the first byte
    if (serialComm.head == 1)
    {
			struct timespec tsp;
            struct termios2 tio;
			clock_gettime(CLOCK_MONOTONIC, &tsp);

            // Log the data (if -v)
            if (serialComm.verbose) logMessage(serialComm.errorState, Out, serialComm.outBuffer, serialComm.outBuffer[PACKLEN_IDX]);

			/*
            * Set 9th data bit to 0, this will remain set for the read, as Linux
            * can't switch between Mark and Space between each read byte.
            */
            usleep(1000);   // Delay to avoid changing parity on the previously written byte.

            ioctl(serialComm.fileDescriptor, TCGETS2, &tio);
			tio.c_cflag &= ~PARODD;
			ioctl(serialComm.fileDescriptor, TCSETS2, &tio);
    }
    // After writing all bytes
    else if (serialComm.head >= serialComm.outBuffer[PACKLEN_IDX])
    {
        if (serialComm.response)
        {
            struct timespec tsp;
            clock_gettime(CLOCK_MONOTONIC, &tsp);
            serialComm.timeout = serialComm.timeoutDuration + tsp.tv_sec;
            serialComm.mode = WAITING_FOR_MARK;
        }
        else
            serialComm.mode = DONE;
        
        serialComm.head = 0;
    }
   
}

void commRead()
{
    struct timespec tsp;
    struct termios  tio;

    // Configuring read timeout
    clock_gettime(CLOCK_MONOTONIC, &tsp);

    // Read and append incoming data to end of the input buffer
    int bytesRead = read(serialComm.fileDescriptor, &serialComm.inBuffer[serialComm.head], sizeof(serialComm.inBuffer) - serialComm.head);

    // Strip framing error stuffing
	for (int i = serialComm.head; i < serialComm.head + bytesRead - 1; i++)
    {
		if (serialComm.inBuffer[i] == 0xFF && serialComm.inBuffer[i+1] == 0xFF)
        {
			memmove(&serialComm.inBuffer[i], &serialComm.inBuffer[i+1], bytesRead - (i - serialComm.head) - 1);
			bytesRead -= 1;
		}
		else if (serialComm.inBuffer[i] == 0xFF && serialComm.inBuffer[i+1] == 0x00)
        {
			memmove(&serialComm.inBuffer[i], &serialComm.inBuffer[i+2], bytesRead - (i - serialComm.head) - 2);
			bytesRead -= 2;
		}
	}

    // Error handling
    if (bytesRead == -1)
    {
        std::cout << "ERROR: bad read \nerrorno: " << strerror(errno) << '\n';
		exit(1);
    }
    else if (bytesRead == 0)    // Timeout case
    {
        serialComm.head = 0;
		serialComm.errorState = TIMEOUT;
        
        if (serialComm.verbose) logMessage(serialComm.errorState, In, serialComm.inBuffer, serialComm.inBuffer[PACKLEN_IDX]);

        ioctl(serialComm.fileDescriptor, TCGETS2, &tio);
		tio.c_cflag |= PARODD;
		ioctl(serialComm.fileDescriptor, TCSETS2, &tio);
		return;
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
        struct termios2 tio;
        serialComm.mode = DONE;
        serialComm.head = 0;

        ioctl(serialComm.fileDescriptor, TCGETS2, &tio);
        tio.c_cflag |= PARODD;
        ioctl(serialComm.fileDescriptor, TCSETS2, &tio);
    }
}
