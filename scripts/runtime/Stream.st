Stream subclass: #PositionableStream instanceVariables: #(collection position readLimit).
PositionableStream subclass: #ReadStream instanceVariables: #().
PositionableStream subclass: #WriteStream instanceVariables: #(writeLimit).
PositionableStream subclass: #LimitedWriteStream instanceVariables: #(limit limitBlock).

Stream ![
nextPut: anElement
    self subclassResponsibility
].

Stream ![
nextPutAll: aCollection
    aCollection do: [:each | self nextPut: each].
    ^ aCollection
].

Stream ![
print: anObject
    anObject printOn: self
].

PositionableStream class ![
on: aCollection
    ^ self basicNew on: aCollection
].

PositionableStream ![
on: aCollection
    collection := aCollection.
    readLimit := aCollection size.
    position := 0.
    self reset
].

PositionableStream ![
reset
    position := 0.
].

PositionableStream ![
originalContents
    ^ collection
].

PositionableStream ![
position
    ^ position
].

PositionableStream ![
contents
    ^ collection copyFrom: 1 to: readLimit
].

WriteStream ![
on: aCollection
    super on: aCollection.
    readLimit := 0.
    writeLimit := aCollection size.
].

WriteStream ![
reset
    readLimit := readLimit max: position.
    position := 0.
].

WriteStream ![
contents
    readLimit := readLimit max: position.
    ^ collection copyFrom: 1 to: position
].

WriteStream ![
nextPut: anElement
    ^ position >= writeLimit
        ifTrue: [self pastEndPut: anElement]
        ifFalse: [
            position := position + 1.
            collection at: position put: anElement
        ]
].

WriteStream ![
lf
    self nextPut: 10 asCharacter
].

WriteStream ![
cr
    self nextPut: 13 asCharacter
].

WriteStream ![
pastEndPut: anObject
	"Grow the collection, then put <anObject> at the current write position."
	collection := collection grownBy: (collection size max: 20).
	writeLimit := collection size.
	collection at: (position := position + 1) put: anObject.
	^ anObject
].

SequenceableCollection class ![
streamSpecies
    self
].

SequenceableCollection class ![
streamContents: aBlock
    | stream |
    stream := WriteStream on: (self streamSpecies new: 100).
    aBlock value: stream.
    ^ stream position = stream originalContents size
        ifTrue: [stream originalContents]
        ifFalse: [stream contents]
].
