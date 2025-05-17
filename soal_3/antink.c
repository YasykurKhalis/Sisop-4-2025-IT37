#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h>

#define BASE_DIR "/it24_host"
#define LOG_FILE "/var/log/it24.log"

int is_bad_file(const char *name) {
    return strstr(name, "nafis") || strstr(name, "kimcun");
}

void reverse_string(char *str) {
    int len = strlen(str);
    for(int i = 0; i < len/2; ++i) {
        char tmp = str[i];
        str[i] = str[len-1-i];
        str[len-1-i] = tmp;
    }
}

void rot13(char *text) {
    for (int i = 0; text[i]; i++) {
        if ('a' <= text[i] && text[i] <= 'z')
            text[i] = ((text[i] - 'a' + 13) % 26) + 'a';
        else if ('A' <= text[i] && text[i] <= 'Z')
            text[i] = ((text[i] - 'A' + 13) % 26) + 'A';
    }
}

void log_activity(const char *action, const char *path) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(log, "[%04d-%02d-%02d %02d:%02d:%02d] %s: %s\n",
            t->tm_year+1900, t->tm_mon+1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec,
            action, path);

    fclose(log);
}

static int antink_getattr(const char *path, struct stat *stbuf) {
    char full_path[1024];
    sprintf(full_path, "%s%s", BASE_DIR, path);
    if (is_bad_file(path + 1))
        reverse_string((char *)(path + 1));  // adjust filename

    int res = lstat(full_path, stbuf);
    if (res == -1)
        return -errno;
    return 0;
}

static int antink_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi) {
    char dir_path[1024];
    sprintf(dir_path, "%s%s", BASE_DIR, path);
    DIR *dp = opendir(dir_path);
    if (!dp) return -errno;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        char name[256];
        strcpy(name, de->d_name);
        if (is_bad_file(name))
            reverse_string(name);
        filler(buf, name, NULL, 0);
    }

    closedir(dp);
    return 0;
}

static int antink_open(const char *path, struct fuse_file_info *fi) {
    char full_path[1024];
    sprintf(full_path, "%s%s", BASE_DIR, path);
    log_activity("READ", path+1);
    int fd = open(full_path, fi->flags);
    if (fd == -1)
        return -errno;
    close(fd);
    return 0;
}

static int antink_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    char full_path[1024];
    sprintf(full_path, "%s%s", BASE_DIR, path);

    FILE *f = fopen(full_path, "r");
    if (!f) return -errno;

    fseek(f, offset, SEEK_SET);
    size_t res = fread(buf, 1, size, f);
    fclose(f);

    if (!is_bad_file(path + 1))  // Apply ROT13 if not a bad file
        rot13(buf);

    return res;
}

static struct fuse_operations antink_oper = {
    .getattr = antink_getattr,
    .readdir = antink_readdir,
    .open    = antink_open,
    .read    = antink_read,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &antink_oper, NULL);
}
