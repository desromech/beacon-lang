MorphEventVisitor subclass: #Morph instanceVariables: #(bounds owner submorphs color visible).

Morph ![
initialize
    super initialize.
    submorphs := #().
    bounds := 0@0 corner: 50@40.
    color := Color blue.
    visible := true.
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
processEvent: anEvent
    ^ anEvent accept: self
].

Morph ![
renderWithBuilder: renderingBuilder
    renderingBuilder solidRectangle: bounds color: color
].

Morph ![
renderChildrenWithBuilder: renderingBuilder
    1 to: submorphs size do: [:index |
        (submorphs at: index) fullRenderWithBuilder: renderingBuilder
    ]
].

Morph ![
fullRenderWithBuilder: renderingBuilder
    visible ifFalse: [^ self].

    self renderWithBuilder: renderingBuilder.
    self renderChildrenWithBuilder: renderingBuilder.

].

Morph subclass: #BorderedMorph instanceVariables: #(borderColor borderWidth).

BorderedMorph ![
initialize
    super initialize.
    borderColor := Color black.
    borderWidth := 1.
].

BorderedMorph ![
renderWithBuilder: renderingBuilder
    renderingBuilder solidRectangle: bounds color: color borderSize: borderWidth borderColor: borderColor
].
