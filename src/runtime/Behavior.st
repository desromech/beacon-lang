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

Behavior ![
superclass: newSuperclass
    superclass := newSuperclass
].

Behavior ![
methodDict
    ^ methodDict
].

Behavior ![
instSize
    ^ instSize
].

Behavior ![
instSize: newInstanceSize
    instSize := newInstanceSize
].

Behavior ![
objectKind
    ^ objectKind
].

Behavior ![
objectKind: newObjectKind
    objectKind := newObjectKind
].

Behavior ![
slots
    ^ slots
].

Slot ![
name
    ^ name
].

Slot ![
name: aName
    name := aName
].

Slot ![
index
    ^ index
].

Slot ![
index: anIndex
    index := anIndex
].

Behavior ![
    setSlotsWithInstanceVariableNames: variableNames
        | newSlots |
        newSlots := Array basicNew: variableNames basicSize.
        1 to: variableNames size do: [:varNameIndex |
            | slot |
            instSize := instSize + 1.
            slot := Slot basicNew
                name: (variableNames basicAt: varNameIndex);
                index: instSize;
                yourself.

            newSlots basicAt: varNameIndex put: slot
        ].
        slots := newSlots
]