/**
 * @module hash
 *
 * This module provides a framework for cryptographic and non-cryptographic encryption.
 * 
 * Examples,
 * 
 * ```blade-repl
 * %> import hash
 * %> 
 * %> hash.md5('Hello, World')
 * '82bb413746aee42f89dea2b59614f9ef'
 * %> 
 * %> hash.sha256('Hello, World')
 * '03675ac53ff9cd1535ccc7dfcdfa2c458c5218371f418dc136f2d19ac1fbe8a5'
 * %> 
 * %> hash.siphash('mykey', 'Hello, World')
 * 'd8e830a590c92b4c'
 * %> 
 * %> hash.hmac_sha256('mykey', 'Hello, World')
 * '61035d3d2119ffdfd710913bf4161d5fba1c2d9431f7de7ef398d359eb1d2481'
 * %> 
 * %> hash.hmac_sha256(bytes([10, 11, 12]), 'My secure text!')
 * 'd782079145a3476fd4e018d44dd024034fa91f626f7f30f2009200c5ac757723'
 * ```
 * 
 * @copyright 2021, Ore Richard Muyiwa and Blade contributors
 */

import .checksum { * }
import .cipher { * }
import .sha { * }
import .mac { * }
import .sha3 { * }
import .cipher { * }
import .blake { * }

