Object subclass: #Morph instanceVariables: #(bounds owner submorphs color).

Morph ![
initialize
    "super initialize."
    submorphs := #().
    bounds := 0@0 corner: 50@40.
    color := Color blue.
].

Morph ![
renderWithBuilder: renderingBuilder
    renderingBuilder solidRectangle: bounds color: color
].