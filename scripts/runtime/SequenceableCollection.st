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
collect: aBlock
    | result |
    result := self species new: self size
    1 to: self size do: [:i |
        result at: i put: (aBlock value: (self at: i))
    ].
    ^ result
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
