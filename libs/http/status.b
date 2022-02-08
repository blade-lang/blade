#!-- part of the http module


# standard response codes to an Http request

# Informational
/**
 * 100 continue
 */
var CONTINUE = 100

/**
 * 101 switching protocols
 */
var SWITCHING_PROTOCOLS = 101

/**
 * 102 processing
 */
var PROCESSING = 102


# Succcess
/**
 * 200 ok
 */
var OK = 200

/**
 * 201 created
 */
var CREATED = 201

/**
 * 202 accepted
 */
var ACCEPTED = 202

/**
 * 203 non authoritative information
 */
var NON_AUTHORITATIVE_INFORMATION = 203

/**
 * 204 no content
 */
var NO_CONTENT = 204

/**
 * 205 reset content
 */
var RESET_CONTENT = 205

/**
 * 206 partial content
 */
var PARTIAL_CONTENT = 206

/**
 * 207 multi status
 */
var MULTI_STATUS = 207

/**
 * 208 already reported
 */
var ALREADY_REPORTED = 208

/**
 * 226 im used
 */
var IM_USED = 226


# Redirection
/**
 * 300 multiple choices
 */
var MULTIPLE_CHOICES = 300

/**
 * 301 moved permanently
 */
var MOVED_PERMANENTLY = 301

/**
 * 302 found
 */
var FOUND = 302

/**
 * 303 see other
 */
var SEE_OTHER = 303

/**
 * 304 not modified
 */
var NOT_MODIFIED = 304

/**
 * 305 use proxy
 */
var USE_PROXY = 305

/**
 * 307 temporary redirect
 */
var TEMPORARY_REDIRECT = 307

/**
 * 308 permanent redirect
 */
var PERMANENT_REDIRECT = 308


# Client Error
/**
 * 400 bad request
 */
var BAD_REQUEST = 400

/**
 * 401 unauthorized
 */
var UNAUTHORIZED = 401

/**
 * 402 payment required
 */
var PAYMENT_REQUIRED = 402

/**
 * 403 forbidden
 */
var FORBIDDEN = 403

/**
 * 404 not found
 */
var NOT_FOUND = 404

/**
 * 405 method not allowed
 */
var METHOD_NOT_ALLOWED = 405

/**
 * 406 not acceptable
 */
var NOT_ACCEPTABLE = 406

/**
 * 407 proxy authentication required
 */
var PROXY_AUTHENTICATION_REQUIRED = 407

/**
 * 408 request timeout
 */
var REQUEST_TIMEOUT = 408

/**
 * 409 conflict
 */
var CONFLICT = 409

/**
 * 410 gone
 */
var GONE = 410

/**
 * 411 length required
 */
var LENGTH_REQUIRED = 411

/**
 * 412 precondition failed
 */
var PRECONDITION_FAILED = 412

/**
 * 413 payload too large
 */
var PAYLOAD_TOO_LARGE = 413

/**
 * 414 request uri too long
 */
var REQUEST_URI_TOO_LONG = 414

/**
 * 415 unsupported media type
 */
var UNSUPPORTED_MEDIA_TYPE = 415

/**
 * 416 requested range not satisfiable
 */
var REQUESTED_RANGE_NOT_SATISFIABLE = 416

/**
 * 417 expectation failed
 */
var EXPECTATION_FAILED = 417

/**
 * 418 teapot
 */
var TEAPOT = 418

/**
 * 421 misdirected request
 */
var MISDIRECTED_REQUEST = 421

/**
 * 422 unprocessable entity
 */
var UNPROCESSABLE_ENTITY = 422

/**
 * 423 locked
 */
var LOCKED = 423

/**
 * 424 failed dependency
 */
var FAILED_DEPENDENCY = 424

/**
 * 426 upgrade required
 */
var UPGRADE_REQUIRED = 426

/**
 * 428 precondition required
 */
var PRECONDITION_REQUIRED = 428

/**
 * 429 too many requests
 */
var TOO_MANY_REQUESTS = 429

/**
 * 431 request header fields too large
 */
var REQUEST_HEADER_FIELDS_TOO_LARGE = 431

/**
 * 444 connection closed without response
 */
var CONNECTION_CLOSED_WITHOUT_RESPONSE = 444

/**
 * 451 unavailable for legal reasons
 */
var UNAVAILABLE_FOR_LEGAL_REASONS = 451

/**
 * 499 client closed request
 */
var CLIENT_CLOSED_REQUEST = 499


# Server Error
/**
 * 500 internal server error
 */
var INTERNAL_SERVER_ERROR = 500

/**
 * 501 not implemented
 */
var NOT_IMPLEMENTED = 501

/**
 * 502 bad gateway
 */
var BAD_GATEWAY = 502

/**
 * 503 service unavailable
 */
var SERVICE_UNAVAILABLE = 503

/**
 * 504 gateway timeout
 */
var GATEWAY_TIMEOUT = 504

/**
 * 505 http version not supported
 */
var HTTP_VERSION_NOT_SUPPORTED = 505

/**
 * 506 variant also negotiates
 */
var VARIANT_ALSO_NEGOTIATES = 506

/**
 * 507 insufficient storage
 */
var INSUFFICIENT_STORAGE = 507

/**
 * 508 loop detected
 */
var LOOP_DETECTED = 508

/**
 * 510 not extended
 */
var NOT_EXTENDED = 510

/**
 * 511 network authentication required
 */
var NETWORK_AUTHENTICATION_REQUIRED = 511

/**
 * 599 network connect timeout error
 */
var NETWORK_CONNECT_TIMEOUT_ERROR = 599

