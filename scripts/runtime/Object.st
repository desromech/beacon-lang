ProtoObject ![
initialize
    "By default do nothing"
].

Object ![
initialize
    "Do nothing"
].

Object ![
asString
    ^ self printString
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


Object ![
postCopy
].

Object ![
copy
    ^ self shallowCopy postCopy
].
