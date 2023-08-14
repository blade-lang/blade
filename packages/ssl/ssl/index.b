#
# @module ssl
#
# Provides OpenSSL bindings for Blade
# @copyright 2021, Ore Richard Muyiwa and Blade contributors
#

import .constants { * }
import .context { * }
import .ssl { * }
import .bio { * }
import .socket { * }
import .server { * }

/**
 * server(port: int, address: string)
 * 
 * Creates an new TLSServer instance.
 * @return TLSServer
 * @throws Exception, SocketExcepion, HttpException
 */
def server(port, address) {
  return TLSServer(port, address)
}
