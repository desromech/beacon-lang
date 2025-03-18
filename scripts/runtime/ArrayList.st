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

ArrayList ![
collect: aBlock
    | result |
    result := self class new: size.
    1 to: size do: [:index |
        result add: (aBlock value: (array basicAt: index))
    ].
    ^ result
].

ArrayList ![
select: aBlock
    | result value |
    result := self class new: size.
    1 to: size do: [:index |
        value := array basicAt: index.
        (aBlock value: value) ifTrue: [
            result add: value
        ]
    ].
    ^ result
].

ArrayList ![
printOn: aStream
    aStream nextPutAll: 'ArrayList withElements: {'.
    
    1 to: size do: [:index |
        index > 1 ifTrue: [
            aStream nextPutAll: ' . '
        ].
        aStream print: (array at: index)
    ].

    aStream nextPutAll:'}'
].