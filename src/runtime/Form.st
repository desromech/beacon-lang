Form ![
bits
    ^ bits
].

Form ![
bits: aByteArray
    bits := aByteArray
].

Form ![
width
    ^ width
].

Form ![
width: aWidth
    width := aWidth
].

Form ![
height
    ^ height
].

Form ![
height: aHeight
    height := aHeight
].

Form ![
depth
    ^ depth
].

Form ![
depth: aDepth
    depth := aDepth
].

Form ![
pitch
    ^ pitch
].

Form ![
pitch: aPitch
    pitch := aPitch
].

Form ![
extent
    ^ width @ height
].

Form ![
extent: anExtent
    width := anExtent x.
    height := anExtent y.
].

Form class ![
extent: anExtent depth: aDepth
    | pitch |
    pitch := anExtent x * (aDepth // 8).

    ^ self new
        width: anExtent x;
        height: anExtent y;
        depth: aDepth;
        pitch: pitch;
        bits: (ByteArray new: pitch * anExtent y);
        yourself
].
