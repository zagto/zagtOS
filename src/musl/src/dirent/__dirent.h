struct dirent;

struct __dirstream
{
    unsigned int num_entries;
    unsigned int current_entry;
    volatile int lock[1];
    struct dirent *entries;
};
