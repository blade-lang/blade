/* The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 *
 * binary-trees Bird #1 program 
 *
 * contributed by Richard Ore
 * *reset*
*/

class TreeNode {
  TreeNode(left, right) {
    self.left = left
    self.right = right
  }

  check() {
    if self.left == nil return 1
    return 1 + self.left.check() + self.right.check()
  }
}

def bottomUpTree(depth) {
  if depth > 0 {
    return TreeNode(bottomUpTree(depth - 1), bottomUpTree(depth - 1))
  }
  return TreeNode(nil, nil)
}

def main(n) {
  var minDepth = 4
  var maxDepth = max(minDepth + 2, n)
  var stretchDepth = maxDepth + 1

  echo 'stretch tree of depth ' + stretchDepth + '\t check: ' + bottomUpTree(stretchDepth).check()

  var longLivedTree = bottomUpTree(maxDepth)

  iter var depth = minDepth; depth <= maxDepth; depth += 2 {
    var check = 0
    var iterations = 1 << (maxDepth - depth + minDepth)

    iter var i = 1; i <= iterations; i++ {
      var tempTree = bottomUpTree(depth)
      check += tempTree.check()
    }

    echo iterations + '\t trees of depth ' + depth + '\t check: ' + check
  }

  echo 'long lived tree of depth ' + maxDepth + '\t check: ' + longLivedTree.check() + '\n'
}

var start = time()
main(21)
echo '\n\nTime taken: ' + (time() - start)