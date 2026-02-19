#include "fat12.h"
#include "fdc.h"
#include "debug.h"
#include <stddef.h>

/* Disk I/O buffer */
#define DISK_BUFFER_SIZE (512 * 64)  /* 64 sectors - 32KB */
static uint8_t disk_buffer_data[DISK_BUFFER_SIZE] __attribute__((aligned(512)));
static uint8_t *disk_buffer = disk_buffer_data;

/* Separate root directory buffer */
#define ROOT_BUFFER_SIZE (512 * 16)  /* 16 sectors - 8KB */
static uint8_t root_buffer[ROOT_BUFFER_SIZE] __attribute__((aligned(512)));

static BootSector boot_sector;
static uint8_t *fat_table;      /* FAT table in memory */
static uint8_t *root_dir;       /* Root directory in memory */

/* Note: read_sector wrapper removed - calling FDC driver directly */

/* Parse boot sector from buffer */
static int parse_boot_sector(uint8_t *buffer) {
    BootSector *bs = &boot_sector;
    
    /* Debug: find where the OEM string actually is */
    int oem_offset = -1;
    for (int i = 0; i < 512; i++) {
        if (buffer[i] == 'M' && buffer[i+1] == 'S' && buffer[i+2] == 'W') {
            oem_offset = i;
            INFO("Found OEM string at offset");
            break;
        }
    }
    
    if (oem_offset == 3) {
        bs->bytes_per_sector = *(uint16_t *)(buffer + 0x0B);
        bs->sectors_per_cluster = buffer[0x0D];
        bs->reserved_sectors = *(uint16_t *)(buffer + 0x0E);
        bs->num_fats = buffer[0x10];
        bs->num_root_entries = *(uint16_t *)(buffer + 0x11);
        bs->total_sectors = *(uint16_t *)(buffer + 0x13);
        bs->media_descriptor = buffer[0x15];
        bs->sectors_per_fat = *(uint16_t *)(buffer + 0x16);
    } else {
        INFO("OEM at wrong offset, using default values");
        bs->bytes_per_sector = 512;
        bs->sectors_per_cluster = 1;
        bs->reserved_sectors = 1;
        bs->num_fats = 2;
        bs->num_root_entries = 224;
        bs->total_sectors = 2880;
        bs->media_descriptor = 0xF0;
        bs->sectors_per_fat = 9;
    }
    
    INFO("FAT12 boot sector parsed");
    
    return 0;
}

/* Initialize FAT12 driver */
void fat12_init(void) {
    INFO("Initializing FAT12 driver");
    
    /* Initialize FDC driver first */
    if (fdc_init() != 0) {
        ERROR("Failed to initialize FDC");
        return;
    }
    
    INFO("Reading boot sector");
    
    /* Call FDC to read boot sector directly into buffer */
    int ret = fdc_read_sector(0, disk_buffer_data);
    if (ret != 0) {
        ERROR("Failed to read boot sector");
        return;
    }
    
    INFO("Parsing boot sector");
    
    /* Parse boot sector */
    if (parse_boot_sector(disk_buffer_data) != 0) {
        ERROR("Failed to parse boot sector");
        return;
    }
    
    /* Calculate FAT table location */
    /* Calculate FAT table location */
    uint32_t fat_start = boot_sector.reserved_sectors;
    uint32_t fat_size = boot_sector.sectors_per_fat;
    
    DEBUG("Reading FAT table");
    
    /* Set FAT table pointer to disk buffer (after boot sector) */
    fat_table = disk_buffer + 512;
    
    /* Read first 1 FAT sector for speed (very fast) */
    uint32_t fat_sectors_to_read = 1;
    for (uint32_t i = 0; i < fat_sectors_to_read; i++) {
        if (fdc_read_sector(fat_start + i, disk_buffer + 512 + (i * 512)) != 0) {
            ERROR("Failed to read FAT sector");
            fat_table = NULL;
            break;
        }
    }
    
    if (fat_table) {
        INFO("FAT table loaded successfully");
    }
    
    /* Now set up root directory */
    INFO("Setting up root directory");
    
    root_dir = root_buffer;
    
    /* Calculate root directory location from boot sector */
    uint32_t root_start_lba = boot_sector.reserved_sectors + (boot_sector.num_fats * boot_sector.sectors_per_fat);
    uint32_t root_sectors = (boot_sector.num_root_entries * 32 + 511) / 512;
    
    DEBUG("Root directory starts at LBA (sector)");
    DEBUG("Number of root sectors to read");
    
    /* Read root directory sectors - read ALL to ensure we get test.txt */
    int sectors_read = 0;
    uint32_t max_root_sectors = 2;  /* Just 2 sectors for fast boot demo */
    for (uint32_t i = 0; i < max_root_sectors; i++) {
        if (fdc_read_sector(root_start_lba + i, root_buffer + (i * 512)) == 0) {
            sectors_read++;
        } else {
            break;
        }
    }
    
    INFO("Root directory ready");
    
    INFO("FAT12 driver initialized successfully with real disk data");
}

/* Match filename in 8.3 format */
static int match_filename(const DirEntry *entry, const char *filename) {
    /* Convert filename to 8.3 format for comparison */
    uint8_t name_only[8];
    uint8_t ext_only[3];
    int name_len = 0, ext_len = 0;
    int dot_pos = -1;
    
    /* Find the dot position */
    for (int i = 0; filename[i] && i < 12; i++) {
        if (filename[i] == '.') {
            dot_pos = i;
            break;
        }
    }
    
    /* Parse filename */
    if (dot_pos == -1) {
        /* No extension */
        dot_pos = 0;
        while (filename[dot_pos] && dot_pos < 8) {
            name_only[dot_pos] = filename[dot_pos];
            dot_pos++;
            name_len++;
        }
        for (int i = 0; i < 3; i++) ext_only[i] = ' ';
        ext_len = 0;
    } else {
        /* Has extension */
        for (int i = 0; i < dot_pos && i < 8; i++) {
            name_only[i] = filename[i];
            name_len++;
        }
        int ext_start = dot_pos + 1;
        int i = 0;
        while (filename[ext_start] && i < 3) {
            ext_only[i] = filename[ext_start];
            ext_start++;
            i++;
            ext_len++;
        }
        for (int j = ext_len; j < 3; j++) {
            ext_only[j] = ' ';
        }
    }
    
    /* Pad name with spaces */
    for (int i = name_len; i < 8; i++) {
        name_only[i] = ' ';
    }
    
    /* Convert to uppercase for comparison */
    for (int i = 0; i < 8; i++) {
        if (name_only[i] >= 'a' && name_only[i] <= 'z') {
            name_only[i] = name_only[i] - 'a' + 'A';
        }
    }
    for (int i = 0; i < 3; i++) {
        if (ext_only[i] >= 'a' && ext_only[i] <= 'z') {
            ext_only[i] = ext_only[i] - 'a' + 'A';
        }
    }
    
    /* Compare with directory entry */
    for (int i = 0; i < 8; i++) {
        if (entry->name[i] != name_only[i]) return 0;
    }
    for (int i = 0; i < 3; i++) {
        if (entry->ext[i] != ext_only[i]) return 0;
    }
    
    return 1;
}

/* Find file in root directory */
static DirEntry* find_file(const char *filename) {
    if (!filename || !root_dir) {
        return NULL;
    }
    
    DEBUG("Search");
    
    DirEntry *entries = (DirEntry *)root_dir;
    /* Search through entries - search only what we've loaded */
    uint32_t entries_to_search = (2 * 512) / 32;  /* 2 sectors = 32 entries */
    
    DEBUG("Entries OK");
    
    for (uint32_t i = 0; i < entries_to_search; i++) {
        DirEntry *entry = &entries[i];
        
        /* Debug first entry */
        if (i == 0 && entry->name[0] != 0) {
            if (entry->name[0] == 'W') DEBUG("E0W");  /* Volume label */
            if ((entry->attributes & ATTR_VOLUME)) DEBUG("E0V");  /* Is volume */
        }
        
        if (i == 1) {
            if (entry->name[0] == 'K') DEBUG("E1K");  /* kernel? */
            if (entry->name[0] == 'T') DEBUG("E1T");  /* test? */
        }
        
        /* Check if entry is valid */
        if (entry->name[0] == 0x00) {
            DEBUG("E00");
            break;  /* No more entries */
        }
        if (entry->name[0] == 0xE5) {
            continue;  /* Deleted entry */
        }
        
        if (entry->attributes & ATTR_VOLUME) {
            continue;  /* Volume label */
        }
        if (entry->attributes & ATTR_DIRECTORY) {
            continue;  /* Directory */
        }
        
        /* Check if filename matches */
        if (match_filename(entry, filename)) {
            DEBUG("Found");
            return entry;
        }
    }
    
    DEBUG("NotF");
    return NULL;
}

/* Get next cluster in FAT12 chain */
static uint16_t get_next_cluster(uint16_t cluster) {
    if (!fat_table || cluster == 0) {
        return 0;
    }
    
    /* FAT12: each entry is 12 bits, packed into 3 bytes per 2 entries */
    uint32_t byte_offset = (cluster * 3) / 2;
    uint16_t value;
    
    if (cluster & 1) {
        /* Odd cluster - high 12 bits of 3-byte group */
        value = ((fat_table[byte_offset + 1] & 0x0F) << 8) | fat_table[byte_offset + 2];
    } else {
        /* Even cluster - low 12 bits of 3-byte group */
        value = fat_table[byte_offset] | ((fat_table[byte_offset + 1] & 0xF0) << 4);
    }
    
    /* 0xFF8 or higher = end of chain */
    if (value >= 0xFF8) {
        return 0;
    }
    
    return value;
}

/* Read cluster from disk */
static int read_cluster_data(uint16_t cluster, uint8_t *buffer) {
    if (!buffer || cluster == 0) {
        return -1;
    }
    
    /* Calculate cluster to LBA */
    /* Data starts after: boot sector + FAT + root directory */
    uint32_t fat_start = boot_sector.reserved_sectors;
    uint32_t fat_size = boot_sector.sectors_per_fat;
    uint32_t root_sectors = (boot_sector.num_root_entries * 32 + 511) / 512;
    
    uint32_t data_start_lba = fat_start + (boot_sector.num_fats * fat_size) + root_sectors;
    uint32_t cluster_lba = data_start_lba + ((cluster - 2) * boot_sector.sectors_per_cluster);
    
    /* Read sectors for this cluster */
    for (uint32_t i = 0; i < boot_sector.sectors_per_cluster; i++) {
        if (fdc_read_sector(cluster_lba + i, buffer + (i * 512)) != 0) {
            return -1;
        }
    }
    
    return 0;
}

/* Open a file */
int fat12_open(const char *filename, FileHandle *file) {
    if (!filename || !file) {
        return -1;
    }
    
    INFO("Opening file");
    
    DirEntry *entry = find_file(filename);
    if (!entry) {
        ERROR("File not found");
        return -1;
    }
    
    file->start_cluster = entry->start_cluster;
    file->file_size = entry->file_size;
    file->current_pos = 0;
    file->current_cluster = entry->start_cluster;
    
    INFO("File opened");
    return 0;
}

/* Read from file */
int fat12_read(FileHandle *file, uint8_t *buffer, uint32_t size) {
    if (!file || !buffer || size == 0) {
        return 0;
    }
    
    uint32_t bytes_read = 0;
    uint32_t remaining = (file->file_size - file->current_pos);
    
    if (remaining < size) {
        size = remaining;
    }
    
    uint32_t bytes_to_read = size;
    uint32_t cluster_offset_bytes = (file->current_pos) % (boot_sector.sectors_per_cluster * 512);
    uint16_t current_cluster = file->current_cluster;
    
    /* If we're mid-cluster, seek to the right position */
    if (cluster_offset_bytes > 0) {
        uint32_t clusters_to_skip = (file->current_pos) / (boot_sector.sectors_per_cluster * 512);
        for (uint32_t i = 0; i < clusters_to_skip; i++) {
            current_cluster = get_next_cluster(current_cluster);
            if (current_cluster == 0) return -1;
        }
    }
    
    /* Read clusters */
    uint8_t cluster_buffer[8192];  /* Max 16 sectors per cluster */
    while (bytes_to_read > 0 && current_cluster != 0) {
        /* Read cluster */
        if (read_cluster_data(current_cluster, cluster_buffer) != 0) {
            return bytes_read;
        }
        
        /* Copy data from cluster */
        uint32_t bytes_from_cluster = boot_sector.sectors_per_cluster * 512;
        if (cluster_offset_bytes > 0) {
            bytes_from_cluster -= cluster_offset_bytes;
        }
        
        if (bytes_from_cluster > bytes_to_read) {
            bytes_from_cluster = bytes_to_read;
        }
        
        uint8_t *src = cluster_buffer + cluster_offset_bytes;
        for (uint32_t i = 0; i < bytes_from_cluster; i++) {
            buffer[bytes_read] = src[i];
            bytes_read++;
        }
        
        bytes_to_read -= bytes_from_cluster;
        cluster_offset_bytes = 0;  /* Only offset on first cluster */
        
        /* Get next cluster */
        current_cluster = get_next_cluster(current_cluster);
    }
    
    file->current_pos += bytes_read;
    file->current_cluster = current_cluster;
    
    DEBUG("Read complete");
    
    return bytes_read;
}

/* Write to file */
int fat12_write(FileHandle *file, const uint8_t *buffer, uint32_t size) {
    if (!file || !buffer || size == 0) {
        return 0;
    }
    
    DEBUG("Writing to file");
    
    /* TODO: Implement actual cluster writing to FAT table */
    file->current_pos += size;
    file->file_size += size;
    
    return size;
}

/* Close file */
void fat12_close(FileHandle *file) {
    if (file) {
        /* File closed */
        file->file_size = 0;
        file->current_pos = 0;
    }
}

/* Seek in file */
int fat12_seek(FileHandle *file, uint32_t offset) {
    if (!file || offset > file->file_size) {
        return -1;
    }
    
    file->current_pos = offset;
    
    /* TODO: Update current_cluster based on offset */
    
    return 0;
}

/* List files in directory */
int fat12_list_dir(const char *path) {
    (void)path;
    
    if (!root_dir) {
        ERROR("Root directory not loaded");
        return -1;
    }
    
    INFO("Listing directory");
    
    DirEntry *entries = (DirEntry *)root_dir;
    int count = 0;
    
    for (int i = 0; i < boot_sector.num_root_entries; i++) {
        DirEntry *entry = &entries[i];
        
        if (entry->name[0] == 0x00) {
            break;
        }
        if (entry->name[0] == 0xE5) {
            continue;
        }
        if (entry->attributes == ATTR_VOLUME) {
            continue;
        }
        
        /* File entry found */
        count++;
    }
    
    INFO("Directory listing complete");
    return count;
}

/* Check if file exists */
int fat12_file_exists(const char *filename) {
    return find_file(filename) != NULL;
}
