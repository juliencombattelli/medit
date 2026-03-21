#ifndef MEDIT_UTF8_H_
#define MEDIT_UTF8_H_

// #include <stddef.h>
// #include <stdint.h>

// // Returns the byte length of the UTF-8 codepoint whose first byte is `b`.
// // Returns 1 for invalid/continuation bytes (treat as opaque single byte).
// static inline size_t utf8_codepoint_size(unsigned char b)
// {
//     if (b < 0x80) {
//         return 1; // ASCII
//     }
//     if (b < 0xC0) {
//         return 1; // Continuation byte — invalid start, treat as 1
//     }
//     if (b < 0xE0) {
//         return 2;
//     }
//     if (b < 0xF0) {
//         return 3;
//     }
//     return 4;
// }

// // Returns the byte length of the codepoint *ending* at byte_pos (exclusive).
// // i.e. how many bytes to step backward from byte_pos to reach the previous
// // codepoint start. byte_pos must be > 0.
// static inline size_t utf8_prev_codepoint_size(const char* line, size_t byte_pos)
// {
//     size_t n = 1;
//     // Walk back over UTF-8 continuation bytes (10xxxxxx)
//     while (n < 4 && byte_pos > n && ((unsigned char)line[byte_pos - n - 1] & 0xC0) == 0x80) {
//         ++n;
//     }
//     return n;
// }

// // Returns the number of codepoints in [data, data+byte_pos).
// // i.e. converts a byte offset into a codepoint (visual column) index.
// static inline size_t utf8_byte_to_codepoint_index(const char* data, size_t byte_pos)
// {
//     size_t col = 0;
//     size_t i = 0;
//     while (i < byte_pos) {
//         i += utf8_codepoint_size((unsigned char)data[i]);
//         ++col;
//     }
//     return col;
// }

#endif // MEDIT_UTF8_H_
