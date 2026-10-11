#include "line_framing.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "generated/config_in_use.h"

// The line being collected; only the task that reads the UART uses these
static char line[LINE_MAX_LEN + 1];  // room for the ending '\0'
static size_t line_len = 0;

/**
 * @brief Says whether a byte is one of the characters in a list.
 *
 * @param byte       The byte
 * @param characters The list, as text ending in '\0'
 * @return true if the byte is in the list
 */
static bool is_one_of(uint8_t byte, const char *characters)
{
    if (byte == '\0') {
        return false;  // the list's own ending is not one of its characters
    }
    return strchr(characters, byte) != NULL;
}

const char *line_framing_add_byte(uint8_t byte)
{
    if (is_one_of(byte, LINE_END_CHARACTERS)) {
        line[line_len] = '\0';
        line_len = 0;  // the next byte starts a new line
        return line;
    }
    if (is_one_of(byte, LINE_SKIPPED_CHARACTERS)) {
        return NULL;  // left out
    }
    if (line_len == sizeof(line) - 1) {
        return NULL;  // no room left: the byte is not kept, so nothing is written past the buffer
    }

    line[line_len++] = (char)byte;
    return NULL;
}
