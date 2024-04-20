#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t b32;
typedef ssize_t isize;

#define array_len(a)  (isize)(sizeof(a) / sizeof(a[0]))
#define cstr_len(s) (isize)(sizeof(s)-1)
#define handle_error() \
  do { \
    printf("Error %s\n", strerror(errno)); \
    exit(-1); \
  } while (0)

#define GB (1L * 1024 * 1024 * 1024)

struct buffer {
  u8 *data;
  ssize_t len;
  ssize_t cap;
};

/* =========================== */

enum encoding {
  encoding_cp1251 = 0,
  encoding_iso8859_5,
  encoding_koi8,
  encoding_unknown,
};

u32 cp1251_to_unicode_table[] = {
  /* absent 0x0 - 0x7F. Start from 0x80 */
  0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021, 0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
  0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x0, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
  0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7, 0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
  0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7, 0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
  0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
  0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
  0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
  0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
};

isize utf8_from_cp1251(u8 *src, isize src_len, u8 *dest) {
  isize dest_len = 0;
  for (isize i = 0; i < src_len; i++) {
    if (src[i] < 0x80) {
      dest[dest_len] = src[i];
      dest_len++;
    } else {
      u32 codepoint = cp1251_to_unicode_table[src[i]-0x80];
      dest[dest_len++] = 0xC0 | ((codepoint >> 6) & 0x1F);
      dest[dest_len++] = 0x80 | (codepoint & 0x3F);
    }
  }
  return dest_len;
}

isize utf8_from_iso8859_5(u8 *src, isize src_len, u8 *dest) {
  // TODO
  isize dest_len = 0;
  for (isize i = 0; i < src_len; i++) {
    if (src[i] < 0x80) {
      dest[dest_len] = src[i];
      dest_len++;
    } else {
      u32 codepoint = cp1251_to_unicode_table[src[i]-0x80];
      u8 byte_0 = 0xC0 | ((codepoint >> 6) & 0x1F);
      u8 byte_1 = 0x80 | (codepoint & 0x3F);
      dest[dest_len++] = byte_0;
      dest[dest_len++] = byte_1;
    }
  }
  return dest_len;
}

isize utf8_from_koi8(u8 *src, isize src_len, u8 *dest) {
  // TODO
  isize dest_len = 0;
  for (isize i = 0; i < src_len; i++) {
    if (src[i] < 0x80) {
      dest[dest_len] = src[i];
      dest_len++;
    } else {
      u32 codepoint = cp1251_to_unicode_table[src[i]-0x80];
      u8 byte_0 = 0xC0 | ((codepoint >> 6) & 0x1F);
      u8 byte_1 = 0x80 | (codepoint & 0x3F);
      dest[dest_len++] = byte_0;
      dest[dest_len++] = byte_1;
    }
  }
  return dest_len;
}

void app_run(isize memory_capacity, enum encoding source_encoding, char *source_filename, char *dest_filename) {
  struct buffer buffer = {0};
  buffer.cap = memory_capacity;
  buffer.data = malloc(buffer.cap);
  if (buffer.data == NULL) {
    handle_error();
  }

  FILE *file = fopen(source_filename, "rb");
  if (file == NULL) {
    handle_error();
  }
    
  size_t bytes_read = 0;
  while ((bytes_read = fread(buffer.data+buffer.len, 1, buffer.cap-buffer.len, file)) > 0) {
    if (ferror(file)) {
      handle_error();
    }
    buffer.len += bytes_read;
  }

  u8 *output_buffer = &buffer.data[buffer.len];
  isize output_buffer_len = 0;
  if (source_encoding == encoding_cp1251) {
    output_buffer_len = utf8_from_cp1251(buffer.data, buffer.len, output_buffer);
  } else if (source_encoding == encoding_iso8859_5) {
    output_buffer_len = utf8_from_iso8859_5(buffer.data, buffer.len, output_buffer);
  } else if (source_encoding == encoding_koi8) {
    output_buffer_len = utf8_from_koi8(buffer.data, buffer.len, output_buffer);
  }

  if (output_buffer_len == 0) {
    return;
  };

  FILE *out = fopen(dest_filename, "wb");
  if (out == NULL) {
    handle_error();
  }

  size_t total_bytes_write = 0;
  size_t bytes_write = 0;
  while ((bytes_write = fwrite(&output_buffer[total_bytes_write], 1, output_buffer_len-total_bytes_write, out)) > 0) {
    if (ferror(file)) {
      handle_error();
    }
    if (feof(file)) {
      break;
    }
    total_bytes_write += bytes_write;
  }
}

int main(int argc, char **argv) {  
  if (argc != 4) {
    printf("Usage: convert-to-utf8 source_encoding source_filename > utf8_output.txt\n");
    return 0;
  }

  enum encoding source_encoding = encoding_unknown;
  if (strcmp(argv[1], "cp1251") == 0) {
    source_encoding = encoding_cp1251;
  } else if (strcmp(argv[1], "iso-8859-5") == 0) {
    source_encoding = encoding_iso8859_5;
  } else if (strcmp(argv[1], "koi8") == 0) {
    source_encoding = encoding_koi8;
  }
  
  if (source_encoding == encoding_unknown) {
    printf("[ERROR]: Unknown encoding\n");
    return 1;
  }
  
  app_run(1*GB, source_encoding, argv[2], argv[3]);
  
  return 0;
}

