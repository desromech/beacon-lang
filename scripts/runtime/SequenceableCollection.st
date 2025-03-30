Object ![
species
    ^ self class
].

Symbol ![
species
    ^ String
].

SequenceableCollection ![
first
    ^ self at: 1
].

SequenceableCollection ![
second
    ^ self at: 2
].

SequenceableCollection ![
third
    ^ self at: 3
].

SequenceableCollection ![
fourth
    ^ self at: 4
].

SequenceableCollection ![
fifth
    ^ self at: 5
].

SequenceableCollection ![
sixth
    ^ self at: 6
].

SequenceableCollection ![
seventh
    ^ self at: 7
].

SequenceableCollection ![
eighth
    ^ self at: 8
].

SequenceableCollection ![
ninth
    ^ self at: 9
].

SequenceableCollection ![
tenth
    ^ self at: 10
].

SequenceableCollection ![
last
    ^ self at: self size
].

SequenceableCollection ![
copyFrom: start to: stop
    | newSize |
    newSize := stop - start + 1.
    ^ (self species new: newSize)
        replaceFrom: 1 to: newSize with: self startingAt: start
].

SequenceableCollection ![
replaceFrom: start to: stop with: replacement startingAt: repStart
	"This destructively replaces elements from start to stop in the receiver
	starting at index, repStart, in the sequenceable collection,
	replacementCollection. Answer the receiver. No range checks are
	performed."

	| repOff |
	repOff := repStart - start.
	start to: stop do: [:index |
        (replacement at: repOff + index).
        self at: index put: (replacement at: repOff + index)
    ].
].


SequenceableCollection ![
do: aBlock
    1 to: self size do: [:i |
        aBlock value: (self at: i)
    ]
].

SequenceableCollection ![
withIndexDo: aBlock
    1 to: self size do: [:i |
        aBlock value: (self at: i) value: i
    ]
].

SequenceableCollection ![
collect: aBlock
    | result |
    result := self species new: self size
    1 to: self size do: [:i |
        result at: i put: (aBlock value: (self at: i))
    ].
    ^ result
].

SequenceableCollection ![
select: aBlock
	"Optimized version of Collection>>#select:"

	"(#(1 2 3 4) select: [:each | each > 2 ]) >>> #(3 4)"
	^ self species new: self size streamContents: [ :stream |
		1 to: self size do: [ :index |
            | each |
            each := self at: index.
			(aBlock value: each) ifTrue: [ stream nextPut: each ]
        ]
    ]
].

SequenceableCollection ![
beginsWith: aSequenceableCollection
	"Answer true if the receiver starts with the argument collection"

	"(#(1 2 3 4 5) beginsWith: #()) >>> true"
	"(#(1 2 3) beginsWith: #(1 2 3 4 5)) >>> false"
	"(#(1 2 3 4 5) beginsWith: #(0 1 2)) >>> false"
	"(#(1 2 3 4 5) beginsWith: #(1 2 3)) >>> true"

	aSequenceableCollection size == 0 ifTrue: [ ^true ].
	self size < aSequenceableCollection size ifTrue: [^false].
	1 to: aSequenceableCollection size do: [:index |
        | each |
        each := aSequenceableCollection at: index.
        (self at: index) ~= each ifTrue: [^false]
    ].
	^true
].

SequenceableCollection ![
includes: anElement
    1 to: self size do: [:i |
        anElement = (self at: i) ifTrue: [
            ^ true
        ]
    ].

    ^ false
].

SequenceableCollection class ![
ofSize: size
	^ self new: size
].

SequenceableCollection class ![
new: newSize streamContents: blockWithArg
	"A variant of #streamContents: where the initial or even final size is given to optimize memory consumption"

	"(Array new: 3 streamContents: [ :out | 3 timesRepeat: [ out nextPut: 42 ] ]) >>> #(42 42 42)"

	| stream |
	stream := WriteStream on: (self streamSpecies new: newSize).
	blockWithArg value: stream.
	"If the write position of stream is at the end of the internal buffer of stream (originalContents),
	we can return it directly instead of making a copy as contents would do"
	^ stream position = stream originalContents size
		ifTrue: [ stream originalContents ]
		ifFalse: [ stream contents ]
].

SequenceableCollection ![
grownBy: length
	| newCollection size |
	size := self size.
	newCollection := self species ofSize: size + length.
	newCollection replaceFrom: 1 to: size with: self startingAt: 1.
	^ newCollection
].

SequenceableCollection ![
atAllPut: value
    1 to: self size do: [:i |
        self at: i put: value
    ]
].

SequenceableCollection ![
indexOf: value
    1 to: self size do: [:i |
        value == (self at: i) ifTrue: [ ^ i]
    ].

    ^ 0
].
