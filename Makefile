MELLOW ?= build/mellow_bin
PROFILE ?= dev

.PHONY: build all

all: build

$(MELLOW):
	MELLOW=$(MELLOW) ./find-mellow.sh

build: $(MELLOW)
	$(MELLOW) fetch
	$(MELLOW) config
	$(MELLOW) build --profile $(PROFILE)
