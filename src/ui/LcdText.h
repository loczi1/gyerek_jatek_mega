#ifndef GYEREK_JATEK_LCD_TEXT_H
#define GYEREK_JATEK_LCD_TEXT_H

#include <stddef.h>

namespace ui {

enum class Alignment { Left, Centre, Right };

size_t textLength(const char* text, size_t maximum);
size_t blankRow(char* buffer, size_t capacity, size_t width, char marker);
size_t copyClipped(char* destination, size_t capacity, size_t offset,
                   const char* source, size_t maximum);
size_t writeAligned(char* buffer, size_t capacity, size_t offset,
                    size_t width, const char* text, Alignment alignment);
size_t composeNameValue(char* buffer, size_t capacity, size_t width,
                        char marker, const char* name, const char* value);

} // namespace ui

#endif