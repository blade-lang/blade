#!-- part of the imagine module

/**
 * Produces a rounded edge.
 */
var ARC_ARC = 0

/**
 * Same as ARC_ARC.
 */
var ARC_PIE = ARC_ARC

/**
 * Connects the starting and ending angles with a straight line.
 */
var ARC_CHORD = 1

/**
 * Indicates that the arc or chord should be outlined, not filled.
 */
var ARC_NO_FILL = 2

/**
 * Used together with ARC_NO_FILL, indicates that the beginning and 
 * ending angles should be connected to the center; this is a good 
 * way to outline (rather than fill) a 'pie slice'.
 */
var ARC_NO_EDGE = 3
