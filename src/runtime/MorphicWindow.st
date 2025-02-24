Window subclass: #MorphicWindow instanceVariables: #(rootMorph).

MorphicWindow ![
rootMorph: aRootMorph
    rootMorph := aRootMorph
].

Morph ![
openInWindow
    MorphicWindow new
        extent: bounds extent;
        rootMorph: self;
        open
].