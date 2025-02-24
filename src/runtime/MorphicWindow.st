Window subclass: #MorphicWindow instanceVariables: #(rootMorph).

MorphicWindow ![
rootMorph: aRootMorph
    rootMorph := aRootMorph
].

MorphicWindow ![
renderWithBuilder: renderingBuilder
    rootMorph renderWithBuilder: renderingBuilder
].

Morph ![
openInWindow
    MorphicWindow new
        extent: bounds extent;
        rootMorph: self;
        open
].