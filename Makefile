BUILD_DIR := build

args = `arg="$(filter-out $@,$(MAKECMDGOALS))" && echo $${arg:-${1}}`

# Remove all build outputs and intermediate files.
clean:
	@ rm -rf $(BUILD_DIR)


# Compile the Jade interpreter.
release:
	@ rm -rf bird
	@ $(MAKE) -f birdy.mk NAME=bird MODE=release SOURCE_DIR=src
	@ cp build/bird bird # For convenience, copy the interpreter to the top level.


# Compile a debug build of Jade.
debug:
	@ rm -rf birdd
	@ $(MAKE) -f birdy.mk NAME=birdd MODE=debug SOURCE_DIR=src
	@ cp build/birdd birdd # For convenience, copy the interpreter to the top level.


test:
	@ ./birdd tests/$(call args,main).b

# test for release
rtest:
	@ ./bird tests/$(call args,main).b

# module test
mtest:
	@ ./birdd tests/modules/$(call args,main).b

# module release test
gtest:
	@ ./bird tests/modules/$(call args,main).b

internal:
	@ $(MAKE) make_internal
	@ ./make_internal $(call args,main)

tests:
	@ bash run-tests.sh

bench:
	@ ./birdd benchmarks/bench-$(call args,main).b

# benchmark for release
rbench:
	@ ./bird benchmarks/bench-$(call args,main).b


.PHONY: clean release debug test tests
