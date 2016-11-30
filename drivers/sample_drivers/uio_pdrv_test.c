#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE 4096UL
#define PAGE_MASK (PAGE_SIZE - 1)

int main(int argc, char **argv) {
 int fd;
 void *map_base, *virt_addr;
 unsigned long writeval = 0x1;
 off_t target = 0x80000000;

 if ((fd = open("/dev/uio0", O_RDWR | O_SYNC)) == -1)
  return -1;
 /* Map one page */
 map_base = mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
            1 * PAGE_SIZE);
 if (map_base == (void *)-1)
   return -1;

 virt_addr = (void *)((off_t)map_base + (target & MAP_MASK));
 *((unsigned long *)virt_addr) = writeval;

 if (munmap(map_base, MAP_SIZE) == -1)
   return -1;
 close(fd);
 return 0;
}
