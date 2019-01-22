/*
* Copyright (c) 2018 Cable Television Laboratories, Inc. ("CableLabs")
*                    and others.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at:
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Created by Jon Dennis (j.dennis@cablelabs.com)
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <macremapper_ioctl.h>
#include <macremapper_filter_config.h>

#include <mrm_filter_conf_parser.h>

static int
open_driver() {
  int fd;
  fd = open("/proc/macremapctl", O_RDWR);
  if (fd == -1) {
    perror("Failed to open driver");
    _exit(1);
  }
  return fd;
}

static int
show( void ) {
  int fd;
  char buf[1024];
  ssize_t rv;

  fd = open_driver();
  while ((rv = read(fd, buf, sizeof(buf))) > 0) {
    if (write(STDOUT_FILENO, buf, rv) != rv) break;
  }
  close(fd);

  return 0;
}

static int
wipe( void ) {
  int fd;
  fd = open_driver();
  if (ioctl(fd, MRM_WIPERUNCONF) == -1) {
    perror("ioctl(MRM_WIPERUNCONF) failed");
    return 1;
  }
  close(fd);
  return 0;
}

static int
loadfilter(const char * const filter_name, const char * const filename) {
  struct mrm_filter_config filterconf;
  int fd;

  if (filter_name[0] == '\0') {
    fprintf(stderr, "Invalid Filter Name\n");
    return 1;
  }
  if (filter_file_load(&filterconf, filename) != 0) {
    return 1;
  }
  strncpy(filterconf.name, filter_name, sizeof(filterconf.name));

  fd = open_driver();
  if (ioctl(fd, MRM_SETFILTER, &filterconf) == -1) {
    perror("ioctl(MRM_SETFILTER) failed");
    return 1;
  }
  close(fd);
  return 0;
}

static int
rmfilter(const char * const filter_name) {
  struct mrm_filter_config filterconf;
  int fd;

  if (filter_name[0] == '\0') {
    fprintf(stderr, "Invalid Filter Name\n");
    return 1;
  }
  strncpy(filterconf.name, filter_name, sizeof(filterconf.name));

  fd = open_driver();
  if (ioctl(fd, MRM_DELETEFILTER, &filterconf) == -1) {
    perror("ioctl(MRM_DELETEFILTER) failed");
    return 1;
  }
  close(fd);
  return 0;
};

static int
parse_macaddr(unsigned char * const output, const char * const str) {
  unsigned octets[6];
  int i;
  if (sscanf(str, "%x:%x:%x:%x:%x:%x",
                  &octets[0],
                  &octets[1],
                  &octets[2],
                  &octets[3],
                  &octets[4],
                  &octets[5]
  ) != 6) return 0;
  for (i=0; i < 6; i++) output[i] = octets[i];
  return 1;
}

static int
remap(int argc, char **argv) {
  const char *filter_name;
  const char *match_macaddr;
  const char *remap_macaddr;
  const char *replace_ifname;
  int fd;
  struct mrm_remap_entry re;

  if (argc < 5) return 1; /* defensive */

  /* initialize variables and put things into human-readable variable names */
  bzero(&re, sizeof(re));
  filter_name   = argv[2];
  match_macaddr = argv[3];

  /* validate + parse "statically-positioned" parameters */
  if (filter_name[0] == '\0') {
    fprintf(stderr, "Invalid Filter Name\n");
    return 1;
  }
  strncpy(re.filter_name, filter_name, sizeof(re.filter_name));

  if (!parse_macaddr(re.match_macaddr, match_macaddr)) {
    fprintf(stderr, "Invalid Match MAC Address: %s\n", match_macaddr);
    return 1;
  }

  /* parse + validate the remap mac address list one-by-one */
  argc -= 4;
  argv += 4;
  while ( argc > 0 ) {
    /* first put things into human-readable variable names */
    remap_macaddr = *argv;
    --argc; ++argv;
    replace_ifname = NULL;
    if (argc > 0) {
      replace_ifname = *argv;
      if (replace_ifname[0] == '\0') {
        replace_ifname = NULL;
      }
      --argc; ++argv;
    }

    /* then add the replacement to the list... */
    if (re.replace_count >= MRM_MAX_REPLACE) {
      fprintf(stderr, "Too many replacements\n");
      return 1;
    }
    if (!parse_macaddr(re.replace[re.replace_count].macaddr, remap_macaddr)) {
      fprintf(stderr, "Invalid Match MAC Address: %s\n", match_macaddr);
      return 1;
    }
    if (replace_ifname != NULL) {
      strncpy(re.replace[re.replace_count].ifname, replace_ifname, sizeof(re.replace[re.replace_count].ifname));
    }
    ++re.replace_count;
  }

  /* write the configuration to the driver... */
  fd = open_driver();
  if (ioctl(fd, MRM_SETREMAP, &re) == -1) {
    perror("ioctl(MRM_SETREMAP) failed");
    return 1;
  }
  close(fd);
  return 0;
}

static int
rmremap(const char * const match_macaddr) {
  struct mrm_remap_entry re;
  int fd;

  if (!parse_macaddr(re.match_macaddr, match_macaddr)) {
    fprintf(stderr, "Invalid Match MAC Address: %s\n", match_macaddr);
    return 1;
  }

  fd = open_driver();
  if (ioctl(fd, MRM_DELETEREMAP, &re) == -1) {
    perror("ioctl(MRM_DELETEREMAP) failed");
    return 1;
  }
  close(fd);
  return 0;
};

static void
usage( void ) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  $ mrmctl <command>\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  Commands:\n");
  fprintf(stderr, "    . show -- Show The Running Configuration\n");
  fprintf(stderr, "    . wipe -- Completely Blow Away The Running Configuration\n");
  fprintf(stderr, "    . loadfilter <filter_name> <file_name> -- Load in a filter from filter configuration file\n");
  fprintf(stderr, "    . rmfilter <filter_name> -- Delete a filter name\n");
  fprintf(stderr, "    . remap <filter_name> <match_macaddr> <dest_macaddr> [dest_ifname] -- Add a remap\n");
  fprintf(stderr, "    . remap <filter_name> <match_macaddr> <dest_macaddr_1> <dest_ifname_1> <dest_macaddr_N> <dest_ifname_N> -- Add a remap with multiple replacements\n");
  fprintf(stderr, "    . rmremap <match_macaddr> -- Delete a remap\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  Multiple remaps:\n");
  fprintf(stderr, "    It is possible to provide multiple replacements with a remap. However, the 'dest_ifname' "
                      "parameter must be provided with reach remap replacement. This can be an empty string "
                      "if it is not intended to perform an interface move with the MAC address replacement. "
                      "\n");
  _exit(1);
}

int
main( int argc, char *argv[] ) {

  if (argc < 2) usage();

  if (strcmp(argv[1], "show") == 0) {
    return show();
  }
  if (strcmp(argv[1], "wipe") == 0) {
    return wipe();
  }
  if (strcmp(argv[1], "loadfilter") == 0) {
    if (argc != 4) usage();
    return loadfilter(argv[2], argv[3]);
  }
  if (strcmp(argv[1], "rmfilter") == 0) {
    if (argc != 3) usage();
    return rmfilter(argv[2]);
  }
  if (strcmp(argv[1], "remap") == 0) {
    if (argc < 5) usage();
    return remap(argc, argv);
  }
  if (strcmp(argv[1], "rmremap") == 0) {
    if (argc != 3) usage();
    return rmremap(argv[2]);
  }

  usage();

  return 0;
}

