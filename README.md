# Scriptable Base Comm

- [Introduction](#introduction)
- [Installation](#installation)
- [Usage](#usage)
- [Examples](#examples)
- [Troubleshooting](#troubleshooting)
- [Future improvements](#future-improvements)

## Introduction

Scriptable Base Comm was made to communicate to any base microcontroller with greater control over the flow of sent and received commands (without needing a console). 

While other scriptable communication apps exist, they often rely on hard-coded message structures, limiting flexibility for future base boards. Scriptable Base Comm addresses this by using a user-friendly C/C++ scripting interface and configurable message structures via XML files, allowing communication with any base board, similar to the existing Base Comm's `BaseType` feature. Currently supports Linux and Windows platforms.

## Installation
0. A recommended software IDE (Integrated Development Environment) for creating and running scripts is Visual Studio Code along with the "C/C++ extension" inside of it. Not required but highly recommended.  

1. A C/C++ compiler is needed to build the code which can be downloaded [here for Windows](https://code.visualstudio.com/docs/cpp/config-mingw) and [here for Linux](https://code.visualstudio.com/docs/cpp/config-linux). Run the following commands in your favorite terminal to ensure successful installation. 
```
g++ -v
make -v
```
2. [Clone](https://www.atlassian.com/git/tutorials/setting-up-a-repository/git-clone) the latest version in `master` each time you want to create a new script.

## Usage

To effectively create scripts using this program, a basic understanding of C/C++ programming (including objects, functions, and other fundamental concepts) is required. For a comprehensive guide on using the program and understanding XML message structures, please refer to the [documentation](https://bitbucket.org/lifefitnessstash/basecomm-script/src/master/docs/).  
A USB to RS422/RS485 COM cable is needed to be able send messages.  
The following below is a quick start guide.

### Logger

See the macros in `src/logger/log.h`. The log names are formatted by a timestamp and outputted in a format acceptable to the Sniffer file viewer application. For example, `data.0808_232026.log` began on August 8th at 11:20:26 PM.
### Serial Constants

Message timeouts and baud rate can be modified in `src/serial/lf_comm.h`.

### Interface Description 

Majority of your scripted code should go in `main.cpp`. Start by selecting the proper XML file to parse by setting `xmlFile` variable to the base you are working with. You'll notice a global `MessageTable` variable named 'table' declared. A `MessageTable` holds all the possible `Message` structures gathered from the XML file. Use `table.findMessage(std::string)` which takes in a string to find and return the command (TX) and response (RX) `Message` object pointers that you are interested in sending and receiving data respectively. Then, to set a field of a `Message` object (only possible for outgoing messages), use the `bool setField(std::string, T)` which takes in the `dataName` of the specific field and a templatized argument to set it to and return true on success. Finally, use the `comm_error sendMessage()` member function of the `Message` class to actually send the configured command to the COM cable which upon success returns `NONE` or 0 (enum offset).

### Building and running your script

`make clean` to remove all object files and executables (good to run before `make`)  
`make` to build

Windows: `.\main.exe COMXX -v` is the accepted format.  
Linux: `./main /dev/ttyUSBX -v`

'X' is a placeholder for a decimal number.  
`-v` flag enables Sniffer logs output to `logs/`

## Examples

[Actuator ON/OFF Duty Cycle](https://bitbucket.org/lifefitnessstash/basecomm-script/src/actuator_example/main.cpp): Used by reliability team for the lift actuator test regarding Symbio Cross-Trainer actuators. Turns an actuator on a certain time, waits, and keeps repeating.   
[Data Collection: Piezo Sensor](https://bitbucket.org/lifefitnessstash/basecomm-script/src/RAIN-19/main.cpp): Original ticket found [here](https://lfagile.atlassian.net/browse/RAIN-19).

## Troubleshooting

- ["CreateFile() failed with error #"](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea)  
If '#' is 5, this is caused by not having exclusive access to the COM port.  
'#' is 2 means the USB device file is not found.  

- [Permission denied error on when running the script (Linux)](https://arduino.stackexchange.com/questions/74714/arduino-dev-ttyusb0-permission-denied-even-when-user-added-to-group-dialout-o)  
This is caused by not having read/write permissions to the USB device file. The quick solution is to prepend `sudo` like so:  `sudo ./main ...`. The better solution is to add yourself to the `dialout` group linked in the tutorial.
## Future improvements

Note: these are listed in no particular order.

- Improve the XML handler to reject bad message structures (i.e. enforcing DataNames, DataTypes, DataSizes match in number and PackLen makes sense).
- Make `T getField()` more user-friendly by potentially templatizing the MsgField class.
- Create a more efficient Makefile
- Add the following cases to the error handling for logs
    - `ERROR_FIFO_OVERRUN`
    - `ERROR_DATA_PRESENT_BEFORE_NEXT_PACKET`
    - `ERROR_PINSWAP_TIMEOUT`
- Develop a user-friendlier interface by adding some GUI to script commands