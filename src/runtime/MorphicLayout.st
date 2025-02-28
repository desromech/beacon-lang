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
