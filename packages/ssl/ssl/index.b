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
import _ssl

/**
 * Creates an new TLSServer instance.
 * 
 * @param int port
 * @param string? host
 * @return TLSServer
 * @throws Exception, SocketExcepion, HttpException
 */
def server(port, host) {
  return TLSServer(port, host)
}

/**
 * The OpenSSL version.
 *
 * @type string
 */
var version = _ssl.version()
