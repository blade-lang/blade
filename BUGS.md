# BUGS

1. `break` keyword does not work properly in `try...catch...` block.
2. `self` object of a class cannot be used in a simplified arithemetic operation. e.g. `self.test += 5`
    it always cause an unexpected behavior.