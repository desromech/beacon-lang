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
