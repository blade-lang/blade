import _imagine

/**
 * QUANT_LIQ if libimagequant is available, QUANT_JQUANT otherwise.
 */
var QUANT_DEFAULT = _imagine.QUANT_DEFAULT


/**
 * libjpeg's old median cut. Fast, but only uses 16-bit color.
 */
var QUANT_JQUANT = _imagine.QUANT_JQUANT


/**
 * NeuQuant - approximation using Kohonen neural network.
 */
var QUANT_NEUQUANT = _imagine.QUANT_NEUQUANT


/**
 * A combination of algorithms used in libimagequant aiming for the highest quality at cost of speed.
 */
var QUANT_LIQ = _imagine.QUANT_LIQ
