# 
# @module curl
# @copyright 2021, Ore Richard Muyiwa and Blade contributors
# 

import .options { Option }
import .filetypes { FileType }
import .infos { Info }
import .auth { * }
import .curl { * }
import _curl


/**
 * The libcurl version.
 */
var version = _curl.version
