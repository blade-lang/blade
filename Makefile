BUILD_DIR := build

args = `arg="$(filter-out $@,$(MAKECMDGOALS))" && echo $${arg:-${1}}`

default: release

# Remove all build outputs and intermediate files.
clean:
	@ rm -rf $(BUILD_DIR)


# Compile the Bird interpreter.
release:
	@ rm -rf build
	@ $(MAKE) -f birdy.mk NAME=bird MODE=release SOURCE_DIR=src
# @ cp build/bird bird
	@ cp -r libs build/libs


# Compile a debug build of Bird.
debug:
	@ rm -rf build
	@ $(MAKE) -f birdy.mk NAME=birdd MODE=debug SOURCE_DIR=src
# @ cp build/birdd birdd
	@ cp -r libs build/libs


test:
	@ ./build/birdd tests/$(call args,main).b

# test for release
rtest:
	@ ./build/bird tests/$(call args,main).b

# module test
ltest:
	@ ./build/birdd tests/libs/$(call args,main).b

# module release test
gtest:
	@ ./build/bird tests/libs/$(call args,main).b

tests:
	@ bash run-tests.sh

bench:
	@ ./build/birdd benchmarks/bench-$(call args,main).b

# benchmark for release
rbench:
	@ ./build/bird benchmarks/bench-$(call args,main).b


all: release
# 	@ rm -rf build birdd
# 	@ $(MAKE) -f birdy.mk NAME=birdd MODE=release SOURCE_DIR=src
# 	@ cp build/birdd birdd


.PHONY: clean release debug test tests
