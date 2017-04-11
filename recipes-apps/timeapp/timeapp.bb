SUMMARY = "Example of how to build an external Linux kernel module"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=12f884d2ae1ff87c09e5b7ccc2c4ca7e"



SRC_URI = "file://timeapp.c \
           file://Makefile \
	   file://COPYING "

S = "${WORKDIR}"

do_compile() {
  #${CC} timeapp.c -o timeapp ${CFLAGS} 
  make
}

do_install() { 
  install -d ${D}${bindir}
  install -m 0755 timeapp ${D}${bindir}
}
