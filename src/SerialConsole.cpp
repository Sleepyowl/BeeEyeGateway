#include "SerialConsole.h"

#include <freertos/FreeRTOS.h>
#include <Arduino.h>


void SerialConsole::printHelp() {
  Serial.println("Commands:");
  Serial.println("help\t-\tPrints this message");
  for (struct CommandEntry *entry = m_commands; entry < m_commands + m_commandCount; ++entry) {
    Serial.println(entry->help);
  }
}

SerialConsole::SerialConsole(CommandEntry* commandDefinitions, size_t count): 
    m_commands(commandDefinitions),m_commandCount(count) {}

void SerialConsole::worker() {
  vTaskDelay(2000);
  Serial.println("Serial Console is ready");
  Serial.println("(type 'help' for available commands)");

  String line = "";

  while (true) {
    while (Serial.available()) {
      char c = Serial.read();

      if (c == '\n' || c == '\r') {
        if (!line.isEmpty()) {
          int spaceIdx = line.indexOf(' ');
          String cmd = (spaceIdx == -1) ? line : line.substring(0, spaceIdx);
          cmd.trim();
          String arg = (spaceIdx == -1) ? "" : line.substring(spaceIdx + 1);
          arg.trim();

          bool found = false;
          const auto end = m_commands + m_commandCount;
          for (CommandEntry *entry = m_commands; entry < end; ++entry) {
            if (cmd.equalsIgnoreCase(entry->name)) {
              entry->handler(arg.c_str());
              found = true;
              break;
            }
          }

          if (!found) {
            if (!cmd.equalsIgnoreCase("help")) {
                Serial.printf("Unknown command: %s\n", cmd);
            }
            printHelp();
          }

          line = "";
        }
      } else {
        line += c;
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void serialConsoleTask(void *param) {
    reinterpret_cast<SerialConsole*>(param)->worker();
}

void SerialConsole::begin() {
    xTaskCreate(serialConsoleTask, "Console", 4096, this, 1, nullptr);
};