#ifndef FAT12_H
#define FAT12_H

#include <stdint.h>

/* FAT12 Boot Sector Structure */
typedef struct {
    uint8_t jump[3];
    uint8_t oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t num_root_entries;
    uint16_t total_sectors;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
} BootSector;

/* FAT12 Directory Entry */
typedef struct {
    uint8_t name[8];
    uint8_t ext[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_tenth;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t access_date;
    uint16_t high_cluster;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t start_cluster;
    uint32_t file_size;
} DirEntry;

/* File attributes */
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME     0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20

/* File handle */
typedef struct {
    uint16_t start_cluster;
    uint32_t file_size;
    uint32_t current_pos;
    uint16_t current_cluster;
} FileHandle;

/* Initialize FAT12 driver */
void fat12_init(void);

/* File operations */
int fat12_open(const char *filename, FileHandle *file);
int fat12_read(FileHandle *file, uint8_t *buffer, uint32_t size);
int fat12_write(FileHandle *file, const uint8_t *buffer, uint32_t size);
void fat12_close(FileHandle *file);
int fat12_seek(FileHandle *file, uint32_t offset);

/* Directory operations */
int fat12_list_dir(const char *path);
int fat12_file_exists(const char *filename);

#endif
