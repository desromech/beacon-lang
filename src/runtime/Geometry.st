Point ![
x
    ^ x
].

Point ![
x: aNumber
    x := aNumber
].

Point ![
y
    ^ y
].

Point ![
y: aNumber
    y := aNumber
].

Point ![
+ aPoint
    ^ (x + aPoint x) @ (y + aPoint y)
].

Point ![
- aPoint
    ^ (x - aPoint x) @ (y - aPoint y)
].

Number ![
@ otherNumber
    ^ Point basicNew
        x: self;
        y: otherNumber;
        yourself
].

Rectangle ![
origin
    ^ origin
].

Rectangle ![
origin: aPoint
    origin := aPoint
].

Rectangle ![
corner
    ^ corner
].

Rectangle ![
corner: aPoint
    corner := aPoint
].

Rectangle ![
extent
    ^ corner - origin
].

Rectangle ![
min
    ^ origin
].

Rectangle ![
max
    ^ corner
].

Rectangle ![
scaledWithXFactor: scaleFactorX y: scaleFactorY
    | scaledOrigin scaledCorner |
    scaledOrigin := (origin x * scaleFactorX) @ (origin y * scaleFactorY).
    scaledCorner := (corner x * scaleFactorX) @ (corner y * scaleFactorY).
    ^ scaledOrigin corner: scaledCorner
].

Point ![
corner: aCornerPoint
    ^ Rectangle basicNew
        origin: self;
        corner: aCornerPoint;
        yourself
].

Point ![
extent: anExtentPoint
    ^ Rectangle basicNew
        origin: self;
        corner: self + anExtentPoint;
        yourself
].
