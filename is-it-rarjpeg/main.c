#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t b32;
typedef ssize_t isize;

#define array_len(a)  (isize)(sizeof(a) / sizeof(a[0]))
#define cstr_len(s) (isize)(sizeof(s)-1)
#define handle_error()                          \
    ({                                          \
        printf("Error %s\n", strerror(errno));  \
        exit(-1);                               \
  })

#define GB (1L * 1024 * 1024 * 1024)

struct buffer {
  u8 *data;
  ssize_t len;
  ssize_t cap;
};

/* struct find_pattern_result { */
/*   isize index; */
/*   b32 found; */
/* }; */

/* struct find_pattern_result find_pattern(u8 *text, isize text_len, u8 *pattern, */
/*                                  isize pattern_len) { */
/*   struct find_pattern_result result = {0}; */
/*   isize text_i = 0; */
/*   isize pattern_j = 0; */
/*   if (text_len == 0) { */
/*     result.found = 0; */
/*     return result; */
/*   } */
/*   if (pattern_len == 0) { */
/*     result.index = 0; */
/*     result.found = 1; */
/*     return result; */
/*   } */

/*   while (1) { */
/*     if (text_i <= (text_len - pattern_len) && (pattern_j < pattern_len) && */
/*         (pattern[pattern_j] == text[text_i + pattern_j])) { */
/*       pattern_j++; */
/*     } else if (text_i <= (text_len - pattern_len) && (pattern_j < pattern_len)) { */
/*       pattern_j = 0; */
/*       text_i++; */
/*     } else */
/*       break; */
/*   } */

/*   result.index = text_i; */
/*   result.found = pattern_j >= pattern_len; */
/*   return result; */
/* } */

u16 read16le(u8* data) {
  u16 result = (u16)data[1];
  result = (result << 8) | (u32)data[0];
  return result;
}

u32 read32le(u8* data) {
  u32 result = (u32)data[3];
  result = (result << 8) | (u32)data[2];
  result = (result << 8) | (u32)data[1];
  result = (result << 8) | (u32)data[0];
  return result;
}

/* Read 16/32 bits little-endian and bump p forward afterwards. */
#define READ16(p) ((p) += 2, read16le((p) - 2))
#define READ32(p) ((p) += 4, read32le((p) - 4))

/* =========================== */

/* End of Central Directory Record. */
struct eocdr {
  u32 signature;
  u16 disk_nbr;        /* Number of this disk. */
  u16 cd_start_disk;   /* Nbr. of disk with start of the CD. */
  u16 disk_cd_entries; /* Nbr. of CD entries on this disk. */
  u16 cd_entries;      /* Nbr. of Central Directory entries. */
  u32 cd_size;         /* Central Directory size in bytes. */
  u32 cd_offset;       /* Central Directory file offset. */
  u16 comment_len;     /* Archive comment length. */
  const u8 *comment;   /* Archive comment. */
};


/* Size of the End of Central Directory Record, not including comment. */
#define EOCDR_BASE_SZ 22
#define EOCDR_SIGNATURE 0x06054b50 /* As little-endian u8[] { 0x50, 0x4b, 0x05, 0x06 } */

b32 read_eocdr(struct eocdr *eocdr, u8 *data, isize len) {
  assert(len >= EOCDR_BASE_SZ);
  u8 *at = data+(len-EOCDR_BASE_SZ);
  u32 maybe_signature = 0;
  isize comment_len = 0;
  for (;;) {
    maybe_signature = read32le(at);
    if (maybe_signature == EOCDR_SIGNATURE) break;
    if (at == data) return 0;
    at--;
    comment_len++;
  }
  eocdr->signature = READ32(at);
  eocdr->disk_nbr = READ16(at);
  eocdr->cd_start_disk = READ16(at);
  eocdr->disk_cd_entries = READ16(at);
  eocdr->cd_entries = READ16(at);
  eocdr->cd_size = READ32(at);
  eocdr->cd_offset = READ32(at);
  eocdr->comment_len = comment_len;
  eocdr->comment = at;
  return 1;
}

int main(int argc, char **argv) {  
  if (argc == 1) {
    printf("Usage: is-it-rarjpeg file.jpg\n");
    return 0;
  }

  FILE *file = fopen(argv[1], "rb");

  if (file == NULL) {
    printf("Fail to open file %s\n", argv[1]);
    return 1;
  }

  struct buffer buffer = {0};
  buffer.cap = 4*GB;
  buffer.data = malloc(buffer.cap);
  if (buffer.data == NULL) {
    perror("malloc");
    goto cleanup;
  }
  
  size_t bytes_read = 0;
  while ((bytes_read = fread(buffer.data+buffer.len, 1, buffer.cap-buffer.len, file)) > 0) {
    buffer.len += bytes_read;
    if (ferror(file)) {
      perror("fread");
      goto cleanup;
    }
  }

  struct eocdr eocdr = {0};
  b32 ok = read_eocdr(&eocdr, buffer.data, buffer.len);
  if (!ok) {
    printf("Given file is not 'rarjpeg'.\n");
    goto cleanup;
  }

  printf("Given file is 'rarjpeg'.\n");

 cleanup:
  fclose(file);
  free(buffer.data);
  return 0;
}
