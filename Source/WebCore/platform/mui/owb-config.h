#ifndef OWB_CONFIG_H
#define OWB_CONFIG_H

#if defined(__aplus__)
#define BOOKMARKLET_INSTALL_PATH "/usr/share/odyssey/bookmarklet/"
#define OWB_DATA "/usr/share/odyssey/conf/owb.conf"
#define INSTALL_PREFIX "/usr"
#define RESOURCE_PATH "/usr/share/odyssey/resource/"
#else
#define BOOKMARKLET_INSTALL_PATH "PROGDIR:bookmarklet/"
#define OWB_DATA "PROGDIR:conf/owb.conf"
#define INSTALL_PREFIX "/usr/local"
#define RESOURCE_PATH "PROGDIR:resource/"
#endif

#endif //OWB_CONFIG_H

