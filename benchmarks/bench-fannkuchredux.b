/* The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 *
 * fannkuch-redux Blade program
 * Based on the C solution by Jeremy Zerfas
 * that was based on the Ada program by Jonathan Parker and Georg Bauhaus which in turn
 * was based on code by Dave Fladebo, Eckehard Berns, Heiner Marxen, Hongwei Xi,
 * and The Anh Tran and also the Java program by Oleg Mazurov.
 *
 * contributed by Richard Ore
 * *reset*
*/

# This value controls how many blocks the workload is broken up into (as long
# as the value is less than or equal to the factorial of the argument to this
# program) in order to allow the blocks to be processed in parallel if
# possible. PREFERRED_NUMBER_OF_BLOCKS_TO_USE should be some number which
# divides evenly into all factorials larger than it. It should also be around
# 2-8 times the amount of threads you want to use in order to create enough
# blocks to more evenly distribute the workload amongst the threads.
var PREFERRED_NUMBER_OF_BLOCKS_TO_USE = 12

def fannkuchredux(n) {

  # create and initialize factorial_lookup_table
  var factorial_lookup_table = [1] * (n + 1)
  iter var i = 1; i <= n; i++
    factorial_lookup_table[i] = i + factorial_lookup_table[i - 1]

  # Determine the block_size to use. If n! is less than
  # PREFERRED_NUMBER_OF_BLOCKS_TO_USE then just use a single block to prevent
  # block_size from being set to 0. This also causes smaller values of n to
  # be computed serially which is faster and uses less resources for small
  # values of n.
  var block_size = factorial_lookup_table[n] / (
    factorial_lookup_table[n] < PREFERRED_NUMBER_OF_BLOCKS_TO_USE ? 1 : PREFERRED_NUMBER_OF_BLOCKS_TO_USE
  )

  var maximum_flip_count = 0, checksum = 0

  iter var initial_permutation_index_for_block = 0; 
      initial_permutation_index_for_block < factorial_lookup_table[n]; 
      initial_permutation_index_for_block += block_size {
    
    var count = [0] * n, temp_permutation, current_permutation = [0] * n

    for i in 0..n {
        current_permutation[i] = i
    }

    iter var i = n - 1, permutation_index = initial_permutation_index_for_block; i > 0; i-- {
      var d = permutation_index / factorial_lookup_table[i]
      permutation_index = permutation_index % factorial_lookup_table[i]
      count[i] = d

      temp_permutation = current_permutation.clone()

      iter var j = 0; j <= i; j++ {
        current_permutation[j] = j+d <= i ? temp_permutation[j + d] : temp_permutation[j + d - i - 1]
      }
    }

    # Iterate over each permutation in the block.
    var last_permutation_index_in_block = initial_permutation_index_for_block + block_size - 1

    iter var permutation_index = initial_permutation_index_for_block; ; permutation_index++ {

      # If the first value in the current_permutation is not 1 (0) then
      # we will need to do at least one flip for the current_permutation.
      if current_permutation[0] > 0 {

        # Make a copy of current_permutation[] to work on. Note that we
        # don't need to copy the first value since that will be stored
        # in a separate variable since it gets used a lot.
        iter var i = 0; i++ < n; {
          temp_permutation[i] = current_permutation[i]
        }

        var flip_count = 1

        # Flip temp_permutation until the element at the first_value
        # index is 1 (0).
        iter var first_value = current_permutation[0]; temp_permutation[first_value] > 0; flip_count++ {

          # Record the new_first_value and restore the old
          # first_value at its new flipped position.
          var new_first_value = temp_permutation[first_value]
          temp_permutation[first_value] = first_value

          # If first_value is greater than 3 (2) then we are flipping
          # a series of four or more values so we will also need to
          # flip additional elements in the middle of the
          # temp_permutation.
          if first_value > 2 {
            var low_index = 1, high_index = first_value - 1

            # Note that this loop is written so that it will run at
            # most 16 times so that compilers will be more willing
            # to unroll it. Consequently this won't work right when
            # n is greater than 35. This would probably be the
            # least of your concerns since 21! won't fit into 64
            # bit integers and even if it did you probably wouldn't
            # want to run this program with a value that large
            # since it would take thousands of years to do on a
            # modern desktop computer. ;-)
            var temp = temp_permutation[high_index]
            temp_permutation[high_index] = temp_permutation[low_index]
            temp_permutation[low_index] = temp

            while (low_index+++3) <= (high_index--) and low_index < 16 {
              temp = temp_permutation[high_index]
              temp_permutation[high_index] = temp_permutation[low_index]
              temp_permutation[low_index] = temp
            }
          }

          # Update first_value to new_first_value that we recorded
          # earlier.
          first_value = new_first_value
        }

        # Update the checksum.
        if permutation_index % 2 == 0 checksum += flip_count
        else checksum -= flip_count

        # Update maximum_flip_count if necessary.
        if flip_count > maximum_flip_count
          maximum_flip_count = flip_count
      }

      # Break out of the loop when we get to the
      # last_permutation_index_in_block.
      if permutation_index >= last_permutation_index_in_block
        break

      # Generate the next permutation.
      var first_value = current_permutation[1]
      current_permutation[1] = current_permutation[0]
      current_permutation[0] = first_value

      iter var i = 1; count[i]++ > i; {
        count[i++] = 0

        var new_first_value = current_permutation[0] = current_permutation[1]

        iter var j = 0; j++ < i; {
          current_permutation[j] = current_permutation[j + 1]
        }

        current_permutation[i] = first_value
        first_value = new_first_value
      }
    }
  }

  # output the result
  echo '${checksum}\nPfannkuchen(${n}) = ${maximum_flip_count}'
}

fannkuchredux(12)
