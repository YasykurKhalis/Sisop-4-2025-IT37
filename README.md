# Sisop-4-2025-IT37
# Soal 1
a. Download zip file dan unzip lalu hapus

Di dalam fungsi `setup_environment()`
```c
if (access(SOURCE_DIR, F_OK) != 0) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "curl -L -o %s \"%s\"", ZIP_NAME, ZIP_URL);
    system(cmd);

    snprintf(cmd, sizeof(cmd), "unzip -o %s", ZIP_NAME);
    system(cmd);

    snprintf(cmd, sizeof(cmd), "rm -f %s", ZIP_NAME);
    system(cmd);
}
```
Penjelasan kode:

If statement akan mengakses directory anomali lalu mengunduh file zip dan meng-unzip file tersebut di dalam directory anomali dan terakhir menghapus file zip. Semua command pertama di simpan dalam buffer menggunakan `snprintf()` dan dieksekusi menggunakan `system()`.

b. Buat program untuk mengonversi string hex menjadi image lalu FUSE dengan directory mount

```c
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
```
Penjelasan kode:

Membuat sistem file berbasis FUSE. File teks berisi hexadecimal otomatis dikonversi menjadi gambar dengan cara:

- Membaca isi hex dan mengubahnya ke format binary

- Menyimpan binary sebagai file `.jpg` di folder `image`

- Log setiap konversi disimpan di `conversion.log`

Konversi ini dijalankan satu kali di awal saat FUSE di-mount melalui fungsi `setup_environment()`. Proses mount tetap menampilkan isi folder `anomali/`, dan semua file bisa diakses secara normal lewat FUSE.

**Fungsi-Fungsi FUSE yang Diimplementasikan**

`hexed_getattr()`
```c
static int hexed_getattr(const char *path, struct stat *stbuf)
```
- Fungsi ini dipanggil setiap kali sistem file ingin mengetahui informasi file (seperti ls -l).

- Fungsi ini akan meneruskan path relatif dari mount point ke direktori sumber (./anomali) dan memanggil lstat() untuk mendapatkan atribut file tersebut.

- Tanpa fungsi ini, FUSE tidak tahu ukuran, tipe, dan permission file.

`hexed_readdir()`
```c
static int hexed_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
```
- Fungsi ini digunakan saat pengguna menjalankan perintah seperti ls pada direktori mount.

- Fungsinya adalah membaca isi direktori anomali dan mengisi daftar file ke buffer menggunakan filler.

- File yang dikembalikan termasuk . dan .., serta semua file dan folder yang ada di anomali.

`hexed_open()`
```c
static int hexed_open(const char *path, struct fuse_file_info *fi)
```
- Fungsi ini menangani proses open() dari sisi FUSE.

- Di sini hanya dicek apakah file tersebut bisa dibuka dengan fopen()—tidak perlu membuka secara nyata karena bacaannya akan dilakukan di read().

`hexed_read()`
```c
static int hexed_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
```
- Fungsi ini dipanggil ketika user membaca isi file, seperti cat file.txt.

- Membaca isi file dari anomali/ ke dalam buffer buf.

- Memastikan hanya data dari offset tertentu yang dibaca, sehingga mendukung sistem pembacaan bertahap.

c. Format nama file hasil konversi
```c
char nama[64];
strncpy(nama, entry->d_name, 64);
char *dot = strrchr(nama, '.');
if (dot) *dot = '\0';
```
Fungsi ini mengambil nama file (misal 1.txt) dan menghilangkan ekstensi .txt, sehingga tinggal "1".

```c
time_t t = time(NULL);
struct tm tm = *localtime(&t);
char waktu[64];
strftime(waktu, sizeof(waktu), "%Y-%m-%d_%H:%M:%S", &tm);
```
Ini menghasilkan waktu saat ini dengan format "YYYY-mm-dd_HH:MM:SS".

```c
char img_path[256];
snprintf(img_path, sizeof(img_path), "%s/%s_image_%s.jpg", IMAGE_DIR, nama, waktu);
```
Membentuk nama file akhir sesuai format soal:
`[nama file]_image_[YYYY-mm-dd]_[HH:MM:SS].jpg`

d. Catat setiap konversi ke log file

```c
FILE *img = fopen(img_path, "wb");
fwrite(bin, 1, bin_len, img);
fclose(img);
free(bin);

FILE *log = fopen(LOG_FILE, "a");
fprintf(log, "[%s]: %s -> %s\n", waktu, entry->d_name, img_path);
fclose(log);
```
Penjelasan kode:

- Mencatat setiap proses konversi file `.txt` (berisi string hexadecimal) menjadi file `.jpg` ke dalam log.

- `img_path` adalah path file gambar hasil konversi.

- waktu berisi timestamp dalam format `YYYY-mm-dd_HH:MM:SS`.

- Setelah proses konversi (`fwrite`), program membuka file log `conversion.log` dengan mode append ("a").

- Lalu mencatat log dalam format seperti ini:
`[2025-05-22_14:45:33]: 1.txt -> ./anomali/image/1_image_2025-05-22_14:45:33.jpg`

Dokumentasi:

Terminal:

![Screenshot 2025-05-22 233121](https://github.com/user-attachments/assets/1853bb62-f10b-4c91-b817-59c4ac5dfbcf)

Setelah dijalankan:

![Screenshot 2025-05-22 233428](https://github.com/user-attachments/assets/9b626ba2-2bd3-40b0-84dd-7df8f34dd090)

Isi log file (sama untuk directory `anomali` dan `mount`):

![Screenshot 2025-05-22 233625](https://github.com/user-attachments/assets/398daf52-7a0a-4fb3-a97a-8a19d3baea9e)


# Soal 2
```c
Conversation opened. 1 unread message.

Skip to content
Using Gmail with screen readers

1 of 939
(no subject)
Inbox

Muhammad Huda Rabbani <mhudarabbani@gmail.com>
Attachments
9:05 PM (1 minute ago)
to me


 One attachment
  •  Scanned by Gmail
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
baymax.c
Displaying baymax.c.
```

Compile 
![image](https://github.com/user-attachments/assets/08b15608-795b-4426-a11d-917a791b8439)


Run Fuse
![image](https://github.com/user-attachments/assets/19223789-b2c5-43de-a6d4-e7b57784e152)


Cek mount_dir
![image](https://github.com/user-attachments/assets/da0da4fe-9501-41d9-b4f5-f471dbb07ea1)


Buka Gambar
![image](https://github.com/user-attachments/assets/342c1ac5-5209-4418-9814-493420a6ce7a)


Copy Gambar
![image](https://github.com/user-attachments/assets/f082f849-9e6c-459e-9b6f-38dd43da3376)


# Soal 3
