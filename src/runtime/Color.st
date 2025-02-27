Color ![
r
    ^ r
].

Color ![
r: aNumber
    r := aNumber
].

Color ![
g
    ^ g
].

Color ![
g: aNumber
    g := aNumber
].

Color ![
b
    ^ b
].

Color ![
b: aNumber
    b := aNumber
].

Color ![
a
    ^ a
].

Color ![
a: aNumber
    a := aNumber
].

Color class ![
r: r g: g b: b a: a
    ^ self basicNew
        r: r asFloat;
        g: g asFloat;
        b: b asFloat;
        a: a asFloat;
        yourself
].

Color class ![
r: r g: g b: b
    ^ self basicNew
        r: r asFloat;
        g: g asFloat;
        b: b asFloat;
        a: 1.0;
        yourself
].

Color class ![
red
    ^ self r: 1 g: 0 b: 0 a: 1
].

Color class ![
green
    ^ self r: 0 g: 1 b: 0 a: 1
].

Color class ![
blue
    ^ self r: 0 g: 0 b: 1 a: 1
].

Color class ![
black
    ^ self r: 0 g: 0 b: 0 a: 1
].

Color class ![
white
    ^ self r: 1 g: 1 b: 1 a: 1
].

Color class ![
transparent
    ^ self r: 0 g: 0 b: 0 a: 0
].
