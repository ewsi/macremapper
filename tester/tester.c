


#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "./macremapper_ioctl.h"

static int
open_ctrl_dev( void ) {
  int result;

  result = open("/proc/macremapctl", O_RDWR);
  if (result == -1) {
    perror("Failed to open ctrl device");
    _exit( 1 );
  }

  return result;
}


static void
test1( const int fd ) {
  unsigned count;

  count = 60;
  if (ioctl(fd, MRM_GETFILTERCOUNT, &count) == -1) {
    perror("ioctl(MRM_GETFILTERCOUNT) failed");
    return;
  }

  printf("Get Filter Count Result: %u\n", count);
}

static void
test2( const int fd ) {
  struct mrm_io_filter iof;

  strncpy(iof.conf.name, "JD's Amazing Filter", sizeof(iof.conf.name));
  iof.conf.rules_active = 0;

  if (ioctl(fd, MRM_SETFILTER, &iof) == -1) {
    perror("ioctl(MRM_SETFILTER) failed");
    return;
  }
}

static void
test3( const int fd ) {

  unsigned id;

  id = 0;
  if (ioctl(fd, MRM_DELETEFILTER, &id) == -1) {
    perror("ioctl(MRM_DELETEFILTER) failed");
    return;
  }

  printf("Deleted filter #%u\n", id);
}

static void
test4( const int fd ) {

  struct mrm_remap_entry r;

  r.match_macaddr[0] = 0xA0;
  r.match_macaddr[1] = 0xCE;
  r.match_macaddr[2] = 0xC8;
  r.match_macaddr[3] = 0x06;
  r.match_macaddr[4] = 0xF9;
  r.match_macaddr[5] = 0xAF;

  r.replace_macaddr[0] = 0xFF;
  r.replace_macaddr[1] = 0xFF;
  r.replace_macaddr[2] = 0xFF;
  r.replace_macaddr[3] = 0xFF;
  r.replace_macaddr[4] = 0xFF;
  r.replace_macaddr[5] = 0xFF;

  strncpy(r.filter_name, "JD's Amazing Filter", sizeof(r.filter_name));

  if (ioctl(fd, MRM_SETREMAP, &r) == -1) {
    perror("ioctl(MRM_SETREMAP) failed");
    return;
  }

  printf("Added Remap Entry\n");
}

static void
test5( const int fd ) {

  unsigned id;

  id = 0;
  if (ioctl(fd, MRM_DELETEREMAP, &id) == -1) {
    perror("ioctl(MRM_DELETEREMAP) failed");
    return;
  }

  printf("Deleted remap #%u\n", id);
}

static void
test6( const int fd ) {
  struct mrm_io_filter iof;

  strncpy(iof.conf.name, "JD's Amazing Filter", sizeof(iof.conf.name));
  iof.conf.rules_active = 1;

  iof.conf.rules[0].payload_size           = 5;
  iof.conf.rules[0].family                 = AF_INET;
  iof.conf.rules[0].proto.match_type       = MRMIPPFILT_MATCHANY;
  iof.conf.rules[0].src_ipaddr.match_type  = MRMIPFILT_MATCHANY;
  iof.conf.rules[0].src_port.match_type    = MRMPORTFILT_MATCHANY;
  iof.conf.rules[0].dst_port.match_type    = MRMPORTFILT_MATCHANY;

  if (ioctl(fd, MRM_SETFILTER, &iof) == -1) {
    perror("ioctl(MRM_SETFILTER) failed");
    return;
  }
}

static void
test7( const int fd ) {

  struct mrm_remap_entry r;

  r.match_macaddr[0] = 0x40;
  r.match_macaddr[1] = 0x16;
  r.match_macaddr[2] = 0x7E;
  r.match_macaddr[3] = 0xB9;
  r.match_macaddr[4] = 0xFB;
  r.match_macaddr[5] = 0x3C;

  r.replace_macaddr[0] = 0xF4;
  r.replace_macaddr[1] = 0x5C;
  r.replace_macaddr[2] = 0x89;
  r.replace_macaddr[3] = 0xBD;
  r.replace_macaddr[4] = 0x37;
  r.replace_macaddr[5] = 0x5D;

  strncpy(r.filter_name, "JD's Amazing Filter", sizeof(r.filter_name));

  if (ioctl(fd, MRM_SETREMAP, &r) == -1) {
    perror("ioctl(MRM_SETREMAP) failed");
    return;
  }

  printf("Added Remap Entry\n");
}

int
main( void ) {
  int fd;

  fd = open_ctrl_dev();
  printf("Opened fd %d\n", fd);

  //test3(fd); return 0;

  //test1(fd);
  test2(fd);
  //test1(fd);

  test4(fd);
  test7(fd);

  //test5(fd);
  //test3(fd);

  test6(fd);

  close(fd);
  return 0;
}
