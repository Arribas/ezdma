#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main()
{
	int uio0_fd;
	int irq_count;
	if ((uio0_fd = open("/dev/uio0", O_RDONLY|O_SYNC)) < 0) {
		perror("open uio0");
	}

	// wait for interrupt
	if (read(uio0_fd, &irq_count, 4) != 4) {
		perror("read uio0");
	}

	printf ("test!!!");

	return 0;
}
