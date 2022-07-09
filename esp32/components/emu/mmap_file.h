//Interface to mmap() a file into memory
int mmap_file(const char *filename, const void **mem);
void munmap_file(const void *ptr);
