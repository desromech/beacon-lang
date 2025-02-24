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
