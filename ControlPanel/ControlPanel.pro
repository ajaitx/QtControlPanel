TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = megaind-rpi \
          main

main.depends = megaind-rpi
