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

Hasil Compile:

![Screenshot 2025-05-22 233121](https://github.com/user-attachments/assets/1853bb62-f10b-4c91-b817-59c4ac5dfbcf)

Setelah dijalankan:

![Screenshot 2025-05-22 233428](https://github.com/user-attachments/assets/9b626ba2-2bd3-40b0-84dd-7df8f34dd090)

Isi log file (sama untuk directory `anomali` dan `mount`):

![Screenshot 2025-05-22 233625](https://github.com/user-attachments/assets/398daf52-7a0a-4fb3-a97a-8a19d3baea9e)


# Soal 2
# Soal 3
