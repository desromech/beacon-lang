ProtoObject ![
initialize
    "By default do nothing"
].

Object ![
initialize
    "Do nothing"
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

Behavior ![
new
    ^ self basicNew initialize
].

Behavior ![
new: anInteger
    ^ (self basicNew: anInteger) initialize
].

Behavior ![
superclass
    ^ superclass
].
