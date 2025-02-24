Object subclass: #Morph instanceVariables: #(bounds owner submorphs color).

Morph ![
initialize
    "super initialize."
    submorphs := #().
    bounds := 0@0 corner: 50@40.
    color := Color blue.
].

Morph ![
bounds
    ^ bounds
].

Morph ![
bounds: aRectangle
    bounds := aRectangle
].

Morph ![
renderWithBuilder: renderingBuilder
    renderingBuilder solidRectangle: bounds color: color
].

Morph subclass: #BorderedMorph instanceVariables: #(borderColor borderWidth).

BorderedMorph ![
initialize
    submorphs := #().
    bounds := 0@0 corner: 50@40.
    color := Color blue.
    borderColor := Color black.
    borderWidth := 1.
]