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
        slots := Array new: variableNames size.
        1 to: variableNames size do: [:varNameIndex |
            | slot |
            self instSize: self instSize + 1.
            slot := Slot new
                name: (variableNames at: varNameIndex);
                index: self instSize;
                yourself.

            slots at: varNameIndex put: slot
        ].
]