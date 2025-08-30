#include "types.h"
#include "user.h"
#include "date.h"
#include "fcntl.h"
int main(int argc, char *argv[])
{
    int fd;
    char buf[512];
    fd = open("ls", O_RDWR);
    read(fd, buf, sizeof(buf));
    write(fd, buf, sizeof(buf));
    exec("ls", argv);
    close(fd);
    exit();
}
