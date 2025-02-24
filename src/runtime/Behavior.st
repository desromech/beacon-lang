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
Behavior ![
    setSlotsWithInstanceVariableNames: variableNames
        | newSlots |
        newSlots := Array basicNew: variableNames basicSize.
        1 to: variableNames basicSize do: [:varNameIndex |
            | slot |
            self instSize: self instSize + 1.
            slot := Slot basicNew
                name: (variableNames basicAt: varNameIndex);
                index: self instSize;
                yourself.

            slots basicAt: varNameIndex put: slot
        ].
        slots := newSlots
]