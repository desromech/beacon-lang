ArrayList class ![
new: aCapacity
    ^ self basicNew initializeWithCapacity: aCapacity
].

ArrayList ![
initialize
    super initialize.
    array := #().
    size := 0.
    capacity := 0.
].

ArrayList ![
initializeWithCapacity: aCapacity
    super initialize.
    array := Array new: aCapacity.
    size := 0.
    capacity := aCapacity.
].

ArrayList ![
size
    ^ size
].

ArrayList ![
at: index
    ^ array basicAt: index
].

ArrayList ![
at: index put: element
    array basicAt: index put: element
].

ArrayList ![
removeAll
    size := 0.
].
