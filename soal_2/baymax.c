#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#define FILENAME "Baymax.jpeg"
#define RELIC_DIR "relics"
#define LOG_FILE "activity.log"
#define MAX_PARTS 14  // Sesuai soal ada 14 pecahan
#define CHUNK_SIZE 1024

// Fungsi untuk logging
void log_activity(const char *action, const char *details) {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);
    
    FILE *log = fopen(LOG_FILE, "a");
    if (log) {
        fprintf(log, "[%s] %s: %s\n", timestamp, action, details);
        fclose(log);
    }
}

long get_total_size() {
    long total = 0;
    for (int i = 0; i < MAX_PARTS; ++i) {
        char path[256];
        snprintf(path, sizeof(path), "%s/%s.%03d", RELIC_DIR, FILENAME, i);
        
        struct stat st;
        if (stat(path, &st) == 0) {
            total += st.st_size;
            log_activity("STAT", path);
        } else {
            log_activity("MISSING", path);
            break;
        }
    }
    return total;
}

static int fs_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));
    
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    
    if (strcmp(path + 1, FILENAME) == 0) {
        long total = get_total_size();
        if (total == 0) return -ENOENT;
        
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = total;
        return 0;
    }
    
    return -ENOENT;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                     off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    
    if (strcmp(path, "/") == 0) {
        long total = get_total_size();
        if (total > 0) {
            filler(buf, FILENAME, NULL, 0);
        }
    }
    
    return 0;
}

static int fs_open(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path + 1, FILENAME) != 0)
        return -ENOENT;
    
    if ((fi->flags & O_ACCMODE) != O_RDONLY)
        return -EACCES;
    
    log_activity("OPEN", path);
    return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset,
                  struct fuse_file_info *fi) {
    if (strcmp(path + 1, FILENAME) != 0)
        return -ENOENT;

    long total_size = get_total_size();
    if (total_size == 0) return -ENOENT;
    
    if (offset >= total_size)
        return 0;
    
    if (offset + size > total_size)
        size = total_size - offset;

    size_t bytes_read = 0;
    size_t global_offset = 0;
    
    for (int i = 0; i < MAX_PARTS && bytes_read < size; ++i) {
        char chunk_path[256];
        snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", RELIC_DIR, FILENAME, i);
        
        struct stat st;
        if (stat(chunk_path, &st) != 0) {
            log_activity("READ_ERROR", chunk_path);
            break;
        }
        
        size_t chunk_size = st.st_size;
        
        // Cek apakah bagian ini mengandung data yang diminta
        if (offset < global_offset + chunk_size) {
            int fd = open(chunk_path, O_RDONLY);
            if (fd == -1) {
                log_activity("OPEN_ERROR", chunk_path);
                continue;
            }
            
            // Hitung offset dalam chunk ini
            off_t chunk_offset = offset > global_offset ? offset - global_offset : 0;
            size_t to_read = chunk_size - chunk_offset;
            
            // Sesuaikan dengan yang diminta
            if (to_read > size - bytes_read)
                to_read = size - bytes_read;
            
            // Baca data
            lseek(fd, chunk_offset, SEEK_SET);
            ssize_t read_bytes = read(fd, buf + bytes_read, to_read);
            close(fd);
            
            if (read_bytes <= 0) {
                log_activity("READ_FAIL", chunk_path);
                break;
            }
            
            bytes_read += read_bytes;
            offset += read_bytes;
            log_activity("READ_PART", chunk_path);
        }
        
        global_offset += chunk_size;
    }
    
    log_activity("READ_DONE", path);
    return bytes_read;
}

static struct fuse_operations ops = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .open = fs_open,
    .read = fs_read,
};

int main(int argc, char *argv[]) {
    // Buat direktori relics jika belum ada
    mkdir(RELIC_DIR, 0755);
    
    // Inisialisasi log
    FILE *log = fopen(LOG_FILE, "w");
    if (log) fclose(log);
    
    log_activity("START", "Mounting FUSE");
    
    return fuse_main(argc, argv, &ops, NULL);
}
