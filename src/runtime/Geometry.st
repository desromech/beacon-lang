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
asPoint
    ^ self
].

Number ![
asPoint
    ^ self @ self
].

Point ![
negated
    ^ (x negated) @ (y negated)
].

Point ![
+ aPointOrNumber
    | aPoint |
    aPoint := aPointOrNumber asPoint.
    ^ (x + aPoint x) @ (y + aPoint y)
].

Point ![
- aPointOrNumber
    | aPoint |
    aPoint := aPointOrNumber asPoint.
    ^ (x - aPoint x) @ (y - aPoint y)
].

Point ![
* aPointOrNumber
    | aPoint |
    aPoint := aPointOrNumber asPoint.
    ^ (x * aPoint x) @ (y * aPoint y)
].

Point ![
// aPointOrNumber
    | aPoint |
    aPoint := aPointOrNumber asPoint.
    ^ (x // aPoint x) @ (y // aPoint y)
].

Point ![
= aPointOrNumber
    | aPoint |
    aPoint := aPointOrNumber asPoint.
    ^ (x = aPoint x) and: [y = aPoint y]
].

Point ![
< aPointOrNumber
    | aPoint |
    aPoint := aPointOrNumber asPoint.
    ^ (x < aPoint x) and: [y < aPoint y]
].

Point ![
<= aPointOrNumber
    | aPoint |
    aPoint := aPointOrNumber asPoint.
    ^ (x <= aPoint x) and: [y <= aPoint y]
].

Point ![
> aPointOrNumber
    | aPoint |
    aPoint := aPointOrNumber asPoint.
    ^ (x > aPoint x) and: [y > aPoint y]
].

Point ![
>= aPointOrNumber
    | aPoint |
    aPoint := aPointOrNumber asPoint.
    ^ (x >= aPoint x) and: [y >= aPoint y]
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
width
    ^ corner x - origin x
].

Rectangle ![
height
    ^ corner y - origin y
].

Rectangle ![
center
    ^ corner + self halfExtent.
].

Rectangle ![
halfExtent
    ^ (corner - origin) // (2@2) .
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
containsPoint: aPoint
    ^ (origin <= aPoint) and: [aPoint <= corner]
].

Rectangle ![
translatedBy: aPoint
    ^ origin + aPoint corner: corner + aPoint
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
