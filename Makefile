.PHONY: dist

all:
	@echo "Please specify a target."

ARCHIVE_NAME = MethclaSampler-$(shell git describe --tags HEAD)

dist:
	git archive --prefix="${ARCHIVE_NAME}/" --format=zip -o "${ARCHIVE_NAME}.zip" -v HEAD

