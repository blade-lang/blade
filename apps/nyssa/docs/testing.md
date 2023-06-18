# Testing

Blade comes shipped with a test runner called `qi` designed to run tests are out the directory `tests` and for this reason. Nyssa provides the default interface to the test runner via the `test` command allowing you to write and run tests for your Blade applications out of the box.

Both Nyssa and `Qi` ship with Blade allowing you write comprehensive tests without any third-party package.

### Writing a simple test

Let's write a test for a hypothetical function that returns the product of two numbers. First, we'll create a file `prod.b` that contains the following code:

```blade
def prod(x, y) {
  return x * y
}
```

Now, let's create a test for it by creating a file `prod.test.b` in the `tests` directory and add the following code to it.

```blade
import ..prod

describe('Product test suite', @() {
  it('should return 6 for 2 and 3', @() {
    expect(prod(2, 3)).to_be(6)
  })
})
```

### Running your tests

Now let's run the test. If you have installed Qi using `nyssa` (which is recommended), then you can run the following command at the root directory (the directory that contains the `tests` folder).

```sh
nyssa test
```

You should get an output similar to this:

```sh
 PASS  tests/prod.test.b
  Product test suite
    ✔ should return 6 for 2 and 3 (1.09ms)
      ✔ expect "6" to be "6"

Test suites:  1 passed, 0 failed, 1 total
Tests:        1 passed, 0 failed, 1 total
Assertions:   1 passed, 0 failed, 1 total
Time:         1.092ms
Ran all test suites.
```

**You have successfully created your first Qi test!**
