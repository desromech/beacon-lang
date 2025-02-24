Object subclass: #Morph instanceVariables: #(bounds owner submorphs).

Morph ![
initialize
    "super initialize."
    submorphs := #().
    bounds := 0@0 corner: 50@40.
].
