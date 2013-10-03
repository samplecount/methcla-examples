.PHONY: dist

all:
	@echo "Please specify a target."

ARCHIVE_NAME = MethclaSampler-$(shell git describe --tags HEAD)

METHCLA_SRC := ../methcla/engine
METHCLA_BUILD_CONFIG := release
NUM_PROCS = $(shell sysctl -n hw.ncpu)

LIBMETHCLA_IPHONE = $(METHCLA_SRC)/build/$(METHCLA_BUILD_CONFIG)/iphone-universal/libmethcla.a
LIBMETHCLA_MACOSX = $(METHCLA_SRC)/build/$(METHCLA_BUILD_CONFIG)/macosx/x86_64/libmethcla-jack.a

.PHONY: $(LIBMETHCLA_IPHONE) $(LIBMETHCLA_MACOSX)

$(LIBMETHCLA_IPHONE):
	cd $(METHCLA_SRC) && ./shake -c $(METHCLA_BUILD_CONFIG) -j$(NUM_PROCS) iphone-universal

$(LIBMETHCLA_MACOSX):
	cd $(METHCLA_SRC) && ./shake -c $(METHCLA_BUILD_CONFIG) -j$(NUM_PROCS) macosx-jack

update-methcla: $(LIBMETHCLA_IPHONE) $(LIBMETHCLA_MACOSX)
	mkdir -p "libs/methcla"
	rsync -av "$(METHCLA_SRC)/include/" "libs/methcla/include"
	mkdir -p "libs/methcla/ios"
	cp $(LIBMETHCLA_IPHONE) "libs/methcla/ios/libmethcla.a"
	mkdir -p "libs/methcla/macosx"
	cp $(LIBMETHCLA_MACOSX) "libs/methcla/macosx/libmethcla-jack.a"

dist:
	git archive --prefix="${ARCHIVE_NAME}/" --format=zip -o "${ARCHIVE_NAME}.zip" -v HEAD
