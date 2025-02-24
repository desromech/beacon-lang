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

Point ![
corner: aCornerPoint
    ^ Rectangle basicNew
        origin: self;
        corner: aCornerPoint;
        yourself
]
