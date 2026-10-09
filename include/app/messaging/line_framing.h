#ifndef LINE_FRAMING_H
#define LINE_FRAMING_H

#include <stdint.h>

#include "generated/config_in_use.h"

// The config for this build says how bytes are cut into lines. There is no fallback.
#ifndef LINE_END_CHARACTERS
#error "LINE_END_CHARACTERS is not set in the config: the characters that end a line, as text"
#endif
#ifndef LINE_SKIPPED_CHARACTERS
#error "LINE_SKIPPED_CHARACTERS is not set in the config: the characters left out of a line, as text"
#endif

/**
 * @brief Adds one received byte to the line being collected.
 *
 * Which characters end a line and which are left out is set in the config: LINE_END_CHARACTERS
 * and LINE_SKIPPED_CHARACTERS. The line is not checked: the sender keeps its lines within
 * LINE_MAX_LEN. Bytes past that are not kept, so a longer line is handed over cut short.
 *
 * @param byte The byte received
 * @return The whole line, ending in '\0' and without the character that ended it, when this byte
 *         ended one; NULL otherwise. The line is valid until the next call.
 */
const char *line_framing_add_byte(uint8_t byte);

#endif
