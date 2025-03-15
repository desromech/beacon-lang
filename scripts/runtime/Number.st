Integer ![
anyMask: mask
    ^ 0 ~= (self & mask)
].

Integer ![
allMask: mask
    ^ mask = (self & mask)
].

Number ![
min: other
    ^ self <= other ifTrue: [self] ifFalse: [other]
].

Number ![
max: other
    ^ self >= other ifTrue: [self] ifFalse: [other]
].

Float class ![
halfPi
    ^ 1.5707963267948966
].

Float class ![
pi
    ^ 3.141592653589793
].

Float class ![
twoPi
    ^ 6.283185307179586
].

Float ![
degreesToRadians
	^ self * 3.141592653589793 / 180.0
].

Number ![
degreesToRadians
	^ self asFloat degreesToRadians
].

Number ![
sqrt
	^ self asFloat sqrt
].

Number ![
exp
	^ self asFloat exp
].

Number ![
ln
	^ self asFloat ln
].

Number ![
sin
	^ self asFloat sin
].

Number ![
cos
	^ self asFloat cos
].

Number ![
tan
	^ self asFloat tan
].

