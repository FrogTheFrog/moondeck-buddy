# Environment Variables Forwarding

This feature allows MoonDeckStream.exe to capture environment variables starting with "APOLLO*" or "SUNSHINE*" and forward them to MoonDeckBuddy.exe for use when launching games.

## How it works

1. **MoonDeckStream.exe** (typically started by streaming software):
   - Captures all environment variables starting with "APOLLO" or "SUNSHINE" 
   - Stores them in shared memory using `EnvSharedMemory` class
   - Clears the shared memory when the application exits

2. **MoonDeckBuddy.exe** (the main application):
   - Retrieves environment variables from shared memory when launching Steam or games
   - Applies these environment variables to the launched processes
   - Logs which environment variables are being used

## Implementation Details

### Files Modified/Created:

1. **`src/lib/utils/include/utils/envsharedmemory.h`** - Header for the shared memory class
2. **`src/lib/utils/envsharedmemory.cpp`** - Implementation of the shared memory class
3. **`src/stream/main.cpp`** - Modified to capture environment variables on startup
4. **`src/lib/os/steamhandler.cpp`** - Modified to use environment variables when launching Steam/games

### Key Classes:

- **`utils::EnvSharedMemory`**: Manages shared memory for environment variable transfer
  - `captureAndStoreEnvironment()`: Captures env vars with specified prefixes
  - `retrieveEnvironment()`: Retrieves env vars from shared memory
  - `hasValidData()`: Checks if valid data exists
  - `clearEnvironment()`: Clears shared memory

### Environment Variable Prefixes:

Currently configured to capture:
- `APOLLO*` - Variables starting with "APOLLO"
- `SUNSHINE*` - Variables starting with "SUNSHINE"

## Usage

No additional configuration is required. The system will automatically:

1. Capture environment variables when MoonDeckStream.exe starts
2. Use those variables when launching games through MoonDeckBuddy.exe
3. Log the activity for debugging purposes

## Logging

The system logs environment variable capture and usage:
- Stream application logs when capturing variables
- Buddy application logs when using variables for game launches
- Debug logging shows individual variable names and values

## Memory Management

- Uses Qt's QSharedMemory for cross-process communication
- Implements data validation with version checking and checksums
- Automatically cleans up shared memory on application exit
- Limited to 64KB maximum data size for safety
