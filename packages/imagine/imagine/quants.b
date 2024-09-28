#!-- part of the imagine module

/**
 * QUANT_LIQ if libimagequant is available, QUANT_JQUANT otherwise.
 */
var QUANT_DEFAULT = 0


/**
 * libjpeg's old median cut. Fast, but only uses 16-bit color.
 */
var QUANT_JQUANT = 1


/**
 * NeuQuant - approximation using Kohonen neural network.
 */
var QUANT_NEUQUANT = 2


/**
 * A combination of algorithms used in libimagequant aiming for the highest quality at cost of speed.
 */
var QUANT_LIQ = 3
