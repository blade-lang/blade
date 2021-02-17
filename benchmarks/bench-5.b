/* The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 *
 * binary-trees Bird #2 program 
 * Alternate solution based on the Python solution
 *
 * contributed by Richard Ore
 * *reset*
*/

class Tree {
  Tree(depth) {
    if depth < 1 {
      self.left = nil
      self.right = nil
    } else {
      depth -= 1
      self.left = Tree(depth)
      self.right = Tree(depth)
    }
  }

  check() {
    if !self.left return 1
    return 1 + self.left.check() + self.right.check()
  }
}

def main(n) {
  var min_depth = 4
  var max_depth = max(min_depth + 2, n)
  var stretch_depth = max_depth + 1

  echo 'stretch tree of depth ${stretch_depth}\t check: ${Tree(stretch_depth).check()}'

  var long_lived_tree = Tree(max_depth)
  var iterations = 2 ** max_depth

  iter var depth = min_depth; depth < stretch_depth; depth += 2 {
    var check = 0
    # var iterations = 1 << (max_depth - depth + min_depth)

    iter var i = 1; i <= iterations; i++ {
      check += Tree(depth).check()
    }

    echo '${iterations}\t trees of depth ${depth}\t check: ${check}'
    iterations //= 4
  }

  echo 'long lived tree of depth ${max_depth}\t check: ${long_lived_tree.check()}'
}

var start = time()
main(21)
echo '\n\nTime taken: ${time() - start}'