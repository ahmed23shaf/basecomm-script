#ifndef COMM_ERRORS_H
#define COMM_ERRORS_H

enum comm_error {
    NONE,           // 0
    BAD_CHECKSUM,   // 1
    TIMEOUT,        // 2
    EMPTY_READ,     // 3
    INVALID_MSG,    // 4
};

#endif // COMM_ERRORS_H