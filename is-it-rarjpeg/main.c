#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

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
  u8 *comment;   /* Archive comment. */
};

/* Size of the End of Central Directory Record, not including comment. */
#define EOCDR_BASE_SZ 22
#define EOCDR_SIGNATURE 0x06054b50 /* As little-endian u8[] { 0x50, 0x4b, 0x05, 0x06 } */

b32 read_eocdr(struct eocdr *eocdr, u8 *data, isize len) {
  assert(len >= EOCDR_BASE_SZ);
  u8 *at = &data[len-EOCDR_BASE_SZ];
  u32 maybe_signature = 0;
  u16 comment_len = 0;
  for (;;) {
    maybe_signature = read32le(at);
    if (maybe_signature == EOCDR_SIGNATURE) break;
    if (at == data) return 0;
    if (comment_len == UINT16_MAX) break;
    at--;
    comment_len++;
  }
  assert(comment_len < UINT16_MAX);
  eocdr->signature = READ32(at);
  eocdr->disk_nbr = READ16(at);
  eocdr->cd_start_disk = READ16(at);
  eocdr->disk_cd_entries = READ16(at);
  eocdr->cd_entries = READ16(at);
  eocdr->cd_size = READ32(at);
  eocdr->cd_offset = READ32(at);
  eocdr->comment_len = READ16(at);
  eocdr->comment = at;
  assert(at = &data[len-comment_len]);
  assert(comment_len == eocdr->comment_len);
  return 1;
}

#define FILE_HEADER_SIGNATURE 0x02014b50

struct file_header {
  u32 signature;
  u16 version_made_by;
  u16 version_needed_to_extract;
  u16 general_purpose_bit_flag;
  u16 compression_method;
  u16 last_mod_file_time;
  u16 last_mod_file_date;
  u32 crc32;
  u32 compressed_size;
  u32 uncompressed_size;
  u16 filename_len;
  u16 extra_field_len;
  u16 file_comment_len;
  u16 disk_number_start;
  u16 internal_file_attributes;
  u32 external_file_attributes;
  u32 relative_offset_of_local_header;
  u8* filename;
  u8* extra_field;
  u8* file_comment;
};

b32 read_file_header(struct file_header *file_header, u8 **beg, u8 *end) {
  u8 *at = *beg;
  b32 found = 0;
  for (;;) {
    if (at == end) break;
    found = read32le(at) == FILE_HEADER_SIGNATURE;
    if (found) break;
    at++;
  }
  if (found) {
    file_header->signature = READ32(at);
    file_header->version_made_by = READ16(at);
    file_header->version_needed_to_extract = READ16(at);
    file_header->general_purpose_bit_flag = READ16(at);
    file_header->compression_method = READ16(at);
    file_header->last_mod_file_time = READ16(at);
    file_header->last_mod_file_date = READ16(at);
    file_header->crc32 = READ32(at);
    file_header->compressed_size = READ32(at);
    file_header->uncompressed_size = READ32(at);
    file_header->filename_len = READ16(at);
    file_header->extra_field_len = READ16(at);
    file_header->file_comment_len = READ16(at);
    file_header->disk_number_start = READ16(at);
    file_header->internal_file_attributes = READ16(at);
    file_header->external_file_attributes = READ32(at);
    file_header->relative_offset_of_local_header = READ32(at);
    file_header->filename = at;
    at += file_header->filename_len;
    file_header->extra_field = at;
    at += file_header->extra_field_len;
    file_header->file_comment = at;
    at += file_header->file_comment_len;
  }
  *beg = at;
  return found;
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

  struct file_header file_header = {0};
  u8 *beg = buffer.data;
  /* u8 *end = &buffer.data[eocdr.cd_offset]; */
  u8* end = &buffer.data[buffer.len];
  for(;;) {
    b32 found = read_file_header(&file_header, &beg, end);
    /* printf("found=%d\n", found); */
    if (!found) break;
    printf("%.*s\n", file_header.filename_len, file_header.filename);
    memset(&file_header, 0, sizeof(struct file_header));
  }

cleanup:
  fclose(file);
  free(buffer.data);
  return 0;
}

