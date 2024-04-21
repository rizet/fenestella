# Styleguide
Any code written in this repository must follow this styleguide.

## EditorConfig
Use of the provided `.editorconfig` file is mandatory. It provides configurations for an editor to automatically make tabs instead of spaces and accomodate other minute details. Use of an editor that parses editor configurations is mandatory.

## Header inclusions
Locally-defined headers (which should be wrapped in quotes) should be included before the compiler headers (which should be wrapped in angle brackets) are included.

As well, relative file paths (such as including `"isr.h"` in the source file `"isr.c"` because they are in the same directory) is incorrect and the full path (e.g. `"cpu/interrupts/isr.h"`) should always be used. 

"Include what you use" doctrine is not mandatory. but preferred when in ambiguity.
```c
/// CORRECT
#include "extra/conv.h"
#include <stdint.h>
#include <stddef.h>

/// INCORRECT
#include "stdint.h"
#include <../extra/conv.h>
#include <stddef.h>
```
