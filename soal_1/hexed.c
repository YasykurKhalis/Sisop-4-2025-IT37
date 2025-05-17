#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>

#define SOURCE_DIR "./anomali"
#define IMAGE_DIR "./anomali/image"
#define LOG_FILE "./anomali/conversion.log"
#define ZIP_URL "https://drive.usercontent.google.com/u/0/uc?id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5&export=download"
#define ZIP_NAME "anomali.zip"

static void setup_environment() {
    if (access(SOURCE_DIR, F_OK) != 0) {
        printf("Mengunduh zip dan menyiapkan direktori...\n");

        char cmd[512];
        snprintf(cmd, sizeof(cmd), "curl -L -o %s \"%s\"", ZIP_NAME, ZIP_URL);
        system(cmd);

        snprintf(cmd, sizeof(cmd), "unzip -o %s", ZIP_NAME);
        system(cmd);

        snprintf(cmd, sizeof(cmd), "rm -f %s", ZIP_NAME);
        system(cmd);
    }

    struct stat st = {0};
    if (stat(IMAGE_DIR, &st) == -1) {
        mkdir(IMAGE_DIR, 0755);
    }

    DIR *dir = opendir(SOURCE_DIR);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && strstr(entry->d_name, ".txt")) {
            char txt_path[512];
            snprintf(txt_path, sizeof(txt_path), "%s/%s", SOURCE_DIR, entry->d_name);

            FILE *fp = fopen(txt_path, "r");
            if (!fp) continue;

            fseek(fp, 0, SEEK_END);
            long len = ftell(fp);
            rewind(fp);

            char *hex = malloc(len + 1);
            fread(hex, 1, len, fp);
            hex[len] = '\0';
            fclose(fp);

            size_t bin_len = len / 2;
            unsigned char *bin = malloc(bin_len);
            for (size_t i = 0; i < bin_len; ++i) {
                sscanf(hex + 2 * i, "%2hhx", &bin[i]);
            }
            free(hex);

            char nama[64];
            strncpy(nama, entry->d_name, 64);
            char *dot = strrchr(nama, '.');
            if (dot) *dot = '\0';

            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            char waktu[64];
            strftime(waktu, sizeof(waktu), "%Y-%m-%d_%H:%M:%S", &tm);

            char img_path[256];
            snprintf(img_path, sizeof(img_path), "%s/%s_image_%s.jpg", IMAGE_DIR, nama, waktu);

            FILE *img = fopen(img_path, "wb");
            fwrite(bin, 1, bin_len, img);
            fclose(img);
            free(bin);

            FILE *log = fopen(LOG_FILE, "a");
            fprintf(log, "[%s]: %s -> %s\n", waktu, entry->d_name, img_path);
            fclose(log);
        }
    }
    closedir(dir);
}

static int hexed_getattr(const char *path, struct stat *stbuf) {
    char real[256];
    snprintf(real, sizeof(real), "%s%s", SOURCE_DIR, path);
    return lstat(real, stbuf);
}

static int hexed_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi) {
    char real[256];
    snprintf(real, sizeof(real), "%s%s", SOURCE_DIR, path);
    DIR *dp = opendir(real);
    if (!dp) return -errno;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        filler(buf, de->d_name, NULL, 0);
    }
    closedir(dp);
    return 0;
}

static int hexed_open(const char *path, struct fuse_file_info *fi) {
    char real[256];
    snprintf(real, sizeof(real), "%s%s", SOURCE_DIR, path);
    FILE *fp = fopen(real, "r");
    if (!fp) return -ENOENT;
    fclose(fp);
    return 0;
}

static int hexed_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
    char real[256];
    snprintf(real, sizeof(real), "%s%s", SOURCE_DIR, path);
    FILE *fp = fopen(real, "r");
    if (!fp) return -ENOENT;

    fseek(fp, 0, SEEK_END);
    size_t len = ftell(fp);
    rewind(fp);

    if (offset < len) {
        if (offset + size > len) size = len - offset;
        fseek(fp, offset, SEEK_SET);
        fread(buf, 1, size, fp);
    } else {
        size = 0;
    }

    fclose(fp);
    return size;
}

static struct fuse_operations hexed_oper = {
    .getattr = hexed_getattr,
    .readdir = hexed_readdir,
    .open    = hexed_open,
    .read    = hexed_read,
};

int main(int argc, char *argv[]) {
    setup_environment();
    return fuse_main(argc, argv, &hexed_oper, NULL);
}
