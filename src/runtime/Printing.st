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
    aStream nextPutAll: name
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
