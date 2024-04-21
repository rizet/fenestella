#pragma once
#include "tty/tty.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void fash_start();
bool fash_started();
void fash_continue();
void fash_execute_command(const char* command);