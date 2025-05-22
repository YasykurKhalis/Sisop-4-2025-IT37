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

c. Format nama file hasil konversi

d. Catat setiap konversi ke log file
# Soal 2
# Soal 3
