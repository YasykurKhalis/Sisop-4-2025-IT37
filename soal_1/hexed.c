#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_PATH 256
#define MAX_FILENAME 128

// soal b
int hexchar_to_int(char c) {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return 10 + (c - 'a');
    if ('A' <= c && c <= 'F') return 10 + (c - 'A');
    return -1;
}

int hexstring_to_bin(const char *hex, unsigned char **output, size_t *output_len) {
    size_t len = strlen(hex);
    if (len % 2 != 0) return -1;

    *output_len = len / 2;
    *output = malloc(*output_len);
    if (!*output) return -1;

    for (size_t i = 0; i < len; i += 2) {
        int hi = hexchar_to_int(hex[i]);
        int lo = hexchar_to_int(hex[i+1]);
        if (hi < 0 || lo < 0) return -1;

        (*output)[i/2] = (hi << 4) | lo;
    }
    return 0;
} // b

int main() {
    // soal a
    const char *zip_url = "https://drive.usercontent.google.com/u/0/uc?id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5&export=download";
    const char *zip_name = "anomali.zip";

    // download zip file
    char download[512];
    snprintf(download, sizeof(download), "curl -L -o %s \"%s\"", zip_name, zip_url);
    if (system(download) != 0) {
        fprintf(stderr, "Gagal mengunduh file.\n");
        return 1;
    }

    // unzip file
    char cmd_unzip[256];
    snprintf(cmd_unzip, sizeof(cmd_unzip), "unzip -o %s", zip_name);
    if (system(cmd_unzip) != 0) {
        fprintf(stderr, "Gagal mengekstrak file zip.\n");
        return 1;
    }

    // delete zip file
    if (remove(zip_name) != 0) {
        perror("Gagal menghapus file zip");
        return 1;
    } // a

    // cek folder image, buat jika belum ada
    struct stat st = {0};
    if (stat("image", &st) == -1) {
        mkdir("image", 0755);
    }

    // soal b
    // buka folder anomali
    DIR *dir = opendir("anomali");
    if (!dir) {
        perror("Gagal membuka folder anomali");
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && strstr(entry->d_name, ".txt")) {
            char safe_filename[MAX_FILENAME];
            strncpy(safe_filename, entry->d_name, MAX_FILENAME - 1);
            safe_filename[MAX_FILENAME - 1] = '\0';

            char input_path[MAX_PATH];
            snprintf(input_path, sizeof(input_path), "anomali/%s", safe_filename);

            // baca file hex
            FILE *fp = fopen(input_path, "r");
            if (!fp) {
                perror("Gagal membuka file txt");
                continue;
            }

            fseek(fp, 0, SEEK_END);
            long fsize = ftell(fp);
            rewind(fp);

            char *hex = malloc(fsize + 1);
            if (!hex) {
                fclose(fp);
                fprintf(stderr, "Gagal alokasi memori.\n");
                continue;
            }

            fread(hex, 1, fsize, fp);
            hex[fsize] = '\0';
            fclose(fp);

            // konversi hex ke binary
            unsigned char *bin = NULL;
            size_t bin_len = 0;
            if (hexstring_to_bin(hex, &bin, &bin_len) != 0) {
                fprintf(stderr, "Gagal konversi hex untuk %s\n", safe_filename);
                free(hex);
                continue;
            }

            // soal c
            // buat nama file output
            char output_name[MAX_FILENAME];
            strncpy(output_name, safe_filename, MAX_FILENAME - 1);
            output_name[MAX_FILENAME - 1] = '\0';
            char *ext = strrchr(output_name, '.');
            if (ext) *ext = '\0'; // hapus ekstensi .txt

            // soal d
            // ambil waktu saat ini
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            
            char date_str[16], time_str[16];
            strftime(date_str, sizeof(date_str), "%Y-%m-%d", &tm);
            strftime(time_str, sizeof(time_str), "%H:%M:%S", &tm);

            // buat nama output file sesuai format soal
            char output_path[MAX_PATH];
            snprintf(output_path, sizeof(output_path), "image/%s_image_%s_%s.jpg", output_name, date_str, time_str);

            // setelah berhasil menulis file gambar:
            FILE *log_fp = fopen("conversion.log", "a");
            if (log_fp) {
                fprintf(log_fp, "[%s][%s]: Successfully converted hexadecimal text %s to %s.\n",
                        date_str, time_str, safe_filename, output_path);
                fclose(log_fp);
            } else {
                perror("Gagal membuka file log");
            } // d

            FILE *out = fopen(output_path, "wb");
            if (out) {
                fwrite(bin, 1, bin_len, out);
                fclose(out);
            } else {
                perror("Gagal menyimpan gambar");
            }

            free(hex);
            free(bin); // c
        }
    }

    closedir(dir); // b

    return 0;
}
