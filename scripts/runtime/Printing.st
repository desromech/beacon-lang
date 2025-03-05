Object ![
printOn: aStream
    aStream nextPutAll: 'a '.
    self class printOn: aStream
].

Object ![
printStringLimitedTo: aLimit
    "TODO: Support the limit."
    ^ String streamContents: [:out | out print: self].
].


Object ![
printString
    ^ self printStringLimitedTo: 50000
].

Class ![
printOn: aStream
    name == nil ifTrue: [
        aStream nextPutAll: 'AnonClass'
    ] ifFalse: [
        aStream nextPutAll: name
    ]
].

Metaclass ![
printOn: aStream
    thisClass printOn: aStream.
    aStream nextPutAll: ' class'
].

UndefinedObject ![
printOn: aStream
    aStream nextPutAll: 'nil'
].

True ![
printOn: aStream
    aStream nextPutAll: 'true'
].

False ![
printOn: aStream
    aStream nextPutAll: 'false'
].

Number ![
printOn: aStream
    aStream nextPutAll: self printString
].

Array ![
printOn: aStream
    aStream nextPut: ${.
    1 to: self size do: [:i |
        i > 1 ifTrue: [
            aStream nextPutAll: '. '
        ].

        aStream print: (self at: i)
    ].
    aStream nextPut: $}.
].

String ![
printOn: aStream
    aStream nextPutAll: self
].

Symbol ![
printOn: aStream
    aStream nextPut: $#; nextPutAll: self
].

Point ![
printOn: aStream
    aStream nextPut: $(; print: x; nextPut: $@; print: y; nextPut: $)
].

Rectangle ![
printOn: aStream
    aStream print: origin; nextPutAll: ' corner: '; print: corner
].