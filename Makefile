export TARGETOS = Linux
export TARGETARCH = AMD

DIRS = CoCo becker FD502 HardDisk mpi orch90 Ramdisk SuperIDE mpu

.PHONY: all subdirs $(DIRS)

all: subdirs

subdirs: $(DIRS)

$(DIRS):
	$(MAKE) -C $@ -f Makefiles/$(TARGETOS)/makefile $(ACTION)

install: ACTION = install
install: subdirs

clean: ACTION = clean
clean: subdirs
