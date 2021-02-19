BUILD_DIR := build

args = `arg="$(filter-out $@,$(MAKECMDGOALS))" && echo $${arg:-${1}}`

# Remove all build outputs and intermediate files.
clean:
	@ rm -rf $(BUILD_DIR)


# Compile the Jade interpreter.
release:
	@ rm -rf bird
	@ $(MAKE) -f birdy.mk NAME=bird MODE=release SOURCE_DIR=src


# Compile a debug build of Jade.
debug:
	@ rm -rf birdd
	@ $(MAKE) -f birdy.mk NAME=birdd MODE=debug SOURCE_DIR=src


test:
	@ ./build/birdd tests/$(call args,main).b

# test for release
rtest:
	@ ./build/bird tests/$(call args,main).b

# module test
mtest:
	@ ./build/birdd tests/modules/$(call args,main).b

# module release test
gtest:
	@ ./build/bird tests/modules/$(call args,main).b

internal:
	@ $(MAKE) make_internal
	@ ./make_internal $(call args,main)

tests:
	@ bash run-tests.sh

bench:
	@ ./build/birdd benchmarks/bench-$(call args,main).b

# benchmark for release
rbench:
	@ ./build/bird benchmarks/bench-$(call args,main).b


.PHONY: clean release debug test tests
