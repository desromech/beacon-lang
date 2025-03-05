BorderedMorph subclass: #Inspector instanceVariables: #(object stringLabel textCodeArea).

Inspector class ![
for: anObject
    ^ self new object: anObject; yourself
].

Inspector class ![
defaultColor
    ^ Color r: 0.8 g: 0.8 b: 0.8 a: 1.0
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

    self layout: (VerticalMorphPackingLayout new
        addMorph: stringLabel proportion: 1;
        addMorph: textCodeArea proportion: 1;
        yourself)
    
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

