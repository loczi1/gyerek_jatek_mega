#include "LcdText.h"
#include <string.h>

namespace ui {

size_t textLength(const char* text, size_t maximum) {
    if (!text) return 0;
    size_t length = 0;
    while (length < maximum && text[length] != '\0') ++length;
    return length;
}

size_t blankRow(char* buffer, size_t capacity, size_t width, char marker) {
    if (!buffer || capacity == 0) return 0;
    const size_t columns = width < capacity ? width : capacity - 1;
    memset(buffer, ' ', columns);
    buffer[columns] = '\0';
    if (columns != 0) buffer[0] = marker;
    return columns;
}

size_t copyClipped(char* destination, size_t capacity, size_t offset,
                   const char* source, size_t maximum) {
    if (!destination || !source || capacity == 0 || offset >= capacity - 1)
        return 0;
    const size_t available = capacity - 1 - offset;
    const size_t limit = maximum < available ? maximum : available;
    const size_t length = textLength(source, limit);
    memcpy(destination + offset, source, length);
    return length;
}

size_t writeAligned(char* buffer, size_t capacity, size_t offset,
                    size_t width, const char* text, Alignment alignment) {
    if (!buffer || capacity == 0 || offset >= capacity - 1) return offset;
    const size_t available = capacity - 1 - offset;
    const size_t columns = width < available ? width : available;
    const size_t length = textLength(text, columns);
    size_t start = offset;
    if (alignment == Alignment::Right) start += columns - length;
    else if (alignment == Alignment::Centre) start += (columns - length) / 2;
    copyClipped(buffer, capacity, start, text, length);
    return start;
}

size_t composeNameValue(char* buffer, size_t capacity, size_t width,
                        char marker, const char* name, const char* value) {
    const size_t columns = blankRow(buffer, capacity, width, marker);
    if (columns <= 1) return columns;
    const size_t contentWidth = columns - 1;
    const size_t valueLength = textLength(value, contentWidth);
    const size_t valueStart = columns - valueLength;
    const size_t nameWidth = valueLength == 0 ? contentWidth :
        (valueStart > 1 ? valueStart - 2 : 0);
    copyClipped(buffer, capacity, 1, name, nameWidth);
    copyClipped(buffer, capacity, valueStart, value, valueLength);
    return valueStart;
}

} // namespace ui