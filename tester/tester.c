


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


#if 0
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
#endif

static void
set_rdkdev_bcast_remap( const int fd ) {

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
unset_rdkdev_bcast_remap( const int fd ) {

  struct mrm_remap_entry r;

  r.match_macaddr[0] = 0xA0;
  r.match_macaddr[1] = 0xCE;
  r.match_macaddr[2] = 0xC8;
  r.match_macaddr[3] = 0x06;
  r.match_macaddr[4] = 0xF9;
  r.match_macaddr[5] = 0xAF;

  if (ioctl(fd, MRM_DELETEREMAP, &r) == -1) {
    perror("ioctl(MRM_DELETEREMAP) failed");
    return;
  }

  printf("Deleted Remap Entry\n");
}

static void
set_laptop_remap( const int fd ) {

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

static void
unset_laptop_remap( const int fd ) {

  struct mrm_remap_entry r;

  r.match_macaddr[0] = 0x40;
  r.match_macaddr[1] = 0x16;
  r.match_macaddr[2] = 0x7E;
  r.match_macaddr[3] = 0xB9;
  r.match_macaddr[4] = 0xFB;
  r.match_macaddr[5] = 0x3C;

  if (ioctl(fd, MRM_DELETEREMAP, &r) == -1) {
    perror("ioctl(MRM_DELETEREMAP) failed");
    return;
  }

  printf("Deleted Remap Entry\n");
}


static void
create_test_filter( const int fd ) {
  struct mrm_io_filter iof;

  strncpy(iof.conf.name, "JD's Amazing Filter", sizeof(iof.conf.name));
  iof.conf.rules_active = 1;

  iof.conf.rules[0].payload_size           = 5;
  iof.conf.rules[0].family                 = AF_INET;
  iof.conf.rules[0].proto.match_type       = MRMIPPFILT_MATCHFAMILY | MRMIPPFILT_MATCHUDP;
  iof.conf.rules[0].src_ipaddr.match_type  = MRMIPFILT_MATCHANY;
  iof.conf.rules[0].src_port.match_type    = MRMPORTFILT_MATCHANY;
  iof.conf.rules[0].dst_port.match_type    = MRMPORTFILT_MATCHANY;

  if (ioctl(fd, MRM_SETFILTER, &iof) == -1) {
    perror("ioctl(MRM_SETFILTER) failed");
    return;
  }
}

static void
usage( void ) {
  fprintf(stderr, "Usage: tester <Command>\n");
  fprintf(stderr, "Commands:\n");
  fprintf(stderr, "  . test_filter -- creates a test filter\n");
  fprintf(stderr, "  . remap_rdkdev -- sets the mac remap (bcast) for the rdk dev machine\n");
  fprintf(stderr, "  . no_remap_rdkdev -- deletes the mac remap (bcast) for the rdk dev machine\n");
  fprintf(stderr, "  . remap_laptop -- sets the mac remap for the test laptop\n");
  fprintf(stderr, "  . no_remap_laptop -- deletes the mac remap for the test laptop\n");
  fflush(stderr);

  _exit(1);
}

int
main( int argc, char *argv[] ) {
  int fd;

  fd = open_ctrl_dev();
  printf("Opened fd %d\n", fd);

  if (argc != 2) usage();

  if (strcmp(argv[1], "test_filter") == 0) {
    create_test_filter(fd);
  }
  else if (strcmp(argv[1], "remap_rdkdev") == 0) {
    set_rdkdev_bcast_remap(fd);
  }
  else if (strcmp(argv[1], "no_remap_rdkdev") == 0) {
    unset_rdkdev_bcast_remap(fd);
  }
  else if (strcmp(argv[1], "remap_laptop") == 0) {
    set_laptop_remap(fd);
  }
  else if (strcmp(argv[1], "no_remap_laptop") == 0) {
    unset_laptop_remap(fd);
  }
  else {
    usage();
  }


  close(fd);
  return 0;
}
