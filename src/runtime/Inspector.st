BorderedMorph subclass: #Inspector instanceVariables: #(object stringLabel textCodeArea).

Inspector class ![
for: anObject
    ^ self new object: anObject; yourself
].

Inspector class ![
defaultColor
    ^ Color white
].

Inspector class ![
defaultExtent
    ^ 200 @ 300
].

Inspector ![
initialize
    super initialize.

    stringLabel := StringMorph new.
    textCodeArea := CodeTextAreaMorph new.
    self addMorph: stringLabel.
    self addMorph: textCodeArea.

    layout := VerticalMorphPackingLayout new
        addMorph: stringLabel proportion: 1;
        addMorph: textCodeArea proportion: 1;
        yourself
].

Inspector ![
object
    ^ object
].

Inspector ![
object: anObject
    object := anObject.
    stringLabel contents: anObject printString
].

