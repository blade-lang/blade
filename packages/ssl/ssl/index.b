/**
 * @module ssl
 *
 * Provides OpenSSL bindings for Blade.
 *
 * @copyright 2021, Ore Richard Muyiwa and Blade contributors
 */

import .constants { * }
import .context { * }
import .ssl { * }
import .bio { * }
import .socket { * }
import .server { * }

/**
 * Creates an new TLSServer instance.
 * 
 * @param int port
 * @param string? host
 * @returns TLSServer
 * @throws Exception, SocketExcepion, HttpException
 */
def server(port, host) {
  return TLSServer(port, host)
}
