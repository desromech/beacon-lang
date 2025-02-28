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
