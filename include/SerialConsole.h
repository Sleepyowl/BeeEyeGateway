#pragma once
#include <stddef.h>

typedef void (*CommandHandler)(const char*);

struct CommandEntry {
  const char* name;
  CommandHandler handler;
  const char* help;
};

class SerialConsole final {
public:
    SerialConsole(CommandEntry* commandDefinitions, size_t count);
    void begin();
private:
    void printHelp();
    void worker();
    CommandEntry    *m_commands;
    size_t          m_commandCount;
    friend void serialConsoleTask(void *param);
};

#define SERIAL_CONSOLE(commandDefs) {commandDefs, sizeof(commandDefs) / sizeof(CommandEntry)}
