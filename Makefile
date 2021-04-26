BUILD_DIR := build

args = `arg="$(filter-out $@,$(MAKECMDGOALS))" && echo $${arg:-${1}}`

default: release

# Remove all build outputs and intermediate files.
clean:
	@ rm -rf $(BUILD_DIR)


# Compile the Jade interpreter.
release:
	@ rm -rf build
	@ $(MAKE) -f birdy.mk NAME=bird MODE=release SOURCE_DIR=src
# @ cp build/bird bird
	@ cp -r libs build/libs


# Compile a debug build of Jade.
debug:
	@ rm -rf build
	@ $(MAKE) -f birdy.mk NAME=birdd MODE=debug SOURCE_DIR=src
# @ cp build/birdd birdd
	@ cp -r libs build/libs


test:
	@ ./birdd tests/$(call args,main).b

# test for release
rtest:
	@ ./bird tests/$(call args,main).b

# module test
ltest:
	@ ./birdd tests/libs/$(call args,main).b

# module release test
gtest:
	@ ./bird tests/libs/$(call args,main).b

tests:
	@ bash run-tests.sh

bench:
	@ ./birdd benchmarks/bench-$(call args,main).b

# benchmark for release
rbench:
	@ ./bird benchmarks/bench-$(call args,main).b


all: release
# 	@ rm -rf build birdd
# 	@ $(MAKE) -f birdy.mk NAME=birdd MODE=release SOURCE_DIR=src
# 	@ cp build/birdd birdd


.PHONY: clean release debug test tests
