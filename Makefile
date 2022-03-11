TOS:=$(shell uname -s)
export TARGETOS = $(TOS)
export TARGETARCH = ARM # ARM or AMD

DIRS = CoCo becker FD502 HardDisk mpi orch90 Ramdisk SuperIDE

ifeq ($(TARGETOS),Linux)
DIRS = CoCo becker FD502 HardDisk mpi orch90 Ramdisk SuperIDE mpu
endif

ifeq ($(TARGETOS),Darwin)
DIRS = CoCo becker FD502 HardDisk mpi orch90 Ramdisk SuperIDE mpu
endif

.PHONY: all subdirs $(DIRS)

all: subdirs

subdirs: $(DIRS)

$(DIRS):
	$(MAKE) -C $@ -f Makefiles/$(TARGETOS)/makefile $(ACTION)

install: ACTION = install
install: subdirs

clean: ACTION = clean
clean: subdirs
