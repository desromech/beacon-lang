ProtoObject ![
initialize
    "By default do nothing"
].

Object ![
initialize
    "Do nothing"
].

Object ![
= other
    ^ self == other
].

Object ![
~= other
    ^ (self = other) not
].

Object ![
size
    ^ self basicSize
].

Object ![
at: index
    ^ self basicAt: index
].

Object ![
at: index put: value
    ^ self basicAt: index put: value
].
