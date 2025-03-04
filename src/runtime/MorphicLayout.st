MorphLayout subclass: #FillMorphLayout instanceVariables: #(morph).
MorphLayout subclass: #MorphPackingLayout instanceVariables: #(elements).
Object subclass: #MorphPackingLayoutElement instanceVariables: #(morph sublayout proportion expanded).
MorphPackingLayout subclass: #HorizontalMorphPackingLayout instanceVariables: #().
MorphPackingLayout subclass: #VerticalMorphPackingLayout instanceVariables: #().

FillMorphLayout ![
applyWithLocalBounds: bounds
    morph bounds: bounds
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
sublayout
    ^ sublayout
].

MorphPackingLayoutElement ![
sublayout: aLayout
    sublayout := aLayout
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

MorphPackingLayoutElement ![
applyRectangle: aRectangle
    morph ~~ nil ifTrue: [
        ^ morph bounds: aRectangle
    ].

    sublayout ~~ nil ifTrue: [
        ^ sublayout applyWithLocalBounds: aRectangle
    ].
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

MorphPackingLayout ![
addLayout: sublayout proportion: proportion
    elements add: (MorphPackingLayoutElement new
        sublayout: sublayout;
        proportion: proportion;
        yourself)
].

HorizontalMorphPackingLayout ![
applyWithLocalBounds: bounds
    | boundsExtent fixedWidth totalProportion eachElement remainingWidth destinationX elementWidth |
    boundsExtent := bounds extent. 
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

    remainingWidth := boundsExtent x - fixedWidth.
    destinationX := bounds origin x.
    1 to: elements size do: [:eachElementIndex |
        eachElement := elements at: eachElementIndex.
        eachElement proportion = 0 ifTrue: [
            eachElement applyRectangle: (destinationX @ bounds origin y extent: eachElement fixedExtent).
            destinationX := destinationX + eachElement fixedExtent y
        ] ifFalse: [
            elementWidth := (remainingWidth * eachElement proportion asFloat / totalProportion + 0.5) asInteger.
            eachElement applyRectangle: (destinationX@bounds origin y extent: elementWidth @ boundsExtent y).
            destinationX := destinationX + elementWidth
        ]
    ].
].

VerticalMorphPackingLayout ![
applyWithLocalBounds: bounds
    | boundsExtent fixedHeight totalProportion eachElement remainingHeight destinationY elementHeight |
    boundsExtent := bounds extent. 
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

    remainingHeight := boundsExtent y - fixedHeight.
    destinationY := bounds origin y.
    1 to: elements size do: [:eachElementIndex |
        eachElement := elements at: eachElementIndex.
        eachElement proportion = 0 ifTrue: [
            eachElement applyRectangle: (bounds origin x @destinationY extent: eachElement fixedExtent).
            destinationY := destinationY + eachElement fixedExtent y
        ] ifFalse: [
            elementHeight := (remainingHeight * eachElement proportion asFloat / totalProportion + 0.5) asInteger.
            eachElement applyRectangle: (bounds origin x @destinationY extent: boundsExtent x @ elementHeight).
            destinationY := destinationY + elementHeight
        ]
    ].
].