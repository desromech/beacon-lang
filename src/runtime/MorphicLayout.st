MorphLayout subclass: #FillMorphLayout instanceVariables: #(morph).
MorphLayout subclass: #MorphPackingLayout instanceVariables: #(elements).
Object subclass: #MorphPackingLayoutElement instanceVariables: #(morph proportion expanded).
MorphPackingLayout subclass: #HorizontalMorphPackingLayout instanceVariables: #().
MorphPackingLayout subclass: #VerticalMorphPackingLayout instanceVariables: #().

FillMorphLayout ![
applyWithExtent: anExtent
    morph extent: anExtent
].

FillMorphLayout ![
morph
    ^ morph
].

FillMorphLayout ![
morph: aMorph
    morph := aMorph
].

MorphPackingLayoutElement ![
morph
    ^ morph
].

MorphPackingLayoutElement ![
morph: aMorph
    morph := aMorph
].

MorphPackingLayoutElement ![
fixedExtent
    morph == nil ifTrue: [^ 0@0].
    ^ morph extent
].

MorphPackingLayoutElement ![
proportion
    ^ proportion
].

MorphPackingLayoutElement ![
proportion: anInteger
    proportion := anInteger
].

MorphPackingLayout ![
initialize
    elements := ArrayList new.
].

MorphPackingLayout ![
addMorph: morph proportion: proportion
    elements add: (MorphPackingLayoutElement new
        morph: morph;
        proportion: proportion;
        yourself)
].

HorizontalMorphPackingLayout ![
applyWithExtent: anExtent
    | fixedWidth totalProportion eachElement remainingWidth destinationX elementWidth |
    fixedWidth := 0.
    totalProportion := 0.
    1 to: elements size do: [:eachElementIndex |
        eachElement := elements at: eachElementIndex.
        eachElement proportion = 0 ifTrue: [
            fixedWidth := fixedWidth + eachElement fixedExtent y
        ] ifFalse: [
            totalProportion := totalProportion + eachElement proportion
        ]
    ].

    remainingWidth := anExtent y - fixedWidth.
    destinationX := 0.
    1 to: elements size do: [:eachElementIndex |
        eachElement := elements at: eachElementIndex.
        eachElement proportion = 0 ifTrue: [
            eachElement morph position: destinationX@0.
            destinationX := destinationX + eachElement fixedExtent y
        ] ifFalse: [
            elementWidth := (remainingWidth * eachElement proportion asFloat / totalProportion + 0.5) asInteger.
            eachElement morph
                position: destinationX@0;
                extent: anExtent x @ elementWidth.
            destinationX := destinationX + elementWidth
        ]
    ].
].

VerticalMorphPackingLayout ![
applyWithExtent: anExtent
    | fixedHeight totalProportion eachElement remainingHeight destinationY elementHeight |
    fixedHeight := 0.
    totalProportion := 0.
    1 to: elements size do: [:eachElementIndex |
        eachElement := elements at: eachElementIndex.
        eachElement proportion = 0 ifTrue: [
            fixedHeight := fixedHeight + eachElement fixedExtent y
        ] ifFalse: [
            totalProportion := totalProportion + eachElement proportion
        ]
    ].

    remainingHeight := anExtent y - fixedHeight.
    destinationY := 0.
    1 to: elements size do: [:eachElementIndex |
        eachElement := elements at: eachElementIndex.
        eachElement proportion = 0 ifTrue: [
            eachElement morph position: 0@destinationY.
            destinationY := destinationY + eachElement fixedExtent y
        ] ifFalse: [
            elementHeight := (remainingHeight * eachElement proportion asFloat / totalProportion + 0.5) asInteger.
            eachElement morph
                position: 0@destinationY;
                extent: anExtent x @ elementHeight.
            destinationY := destinationY + elementHeight
        ]
    ].
].