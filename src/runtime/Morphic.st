MorphEventVisitor subclass: #Morph instanceVariables: #(bounds owner submorphs color visible).

Morph class ![
defaultColor
    ^ Color blue.
].

Morph class ![
defaultBounds
    ^ 0@0 corner: 50@40.
].

Morph ![
initialize
    super initialize.
    submorphs := #().
    bounds := self class defaultBounds.
    color := self class defaultColor.
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
extent
    ^ bounds extent
].


Morph ![
extent: newExtent
    self bounds: (bounds origin extent: newExtent)
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

Morph subclass: #StringMorph instanceVariables: #(backgroundColor fontFace contents).

StringMorph ![
initialize
    super initialize.
    backgroundColor := Color transparent.
    contents := ''.
].

StringMorph class ![
defaultColor
    ^ Color black.
].

StringMorph ![
renderWithBuilder: renderingBuilder
    renderingBuilder
        solidRectangle: bounds color: backgroundColor;
        text: contents rectangle: bounds color: color
].

StringMorph ![
backgroundColor
    ^ backgroundColor
].

StringMorph ![
backgroundColor: aColor
    backgroundColor := aColor
].

StringMorph ![
contents
    ^ contents
].

StringMorph ![
contents: aString
    contents := aString.
    self fitContents
].

StringMorph ![
fitContents
    | textExtent |
    textExtent := Font defaultFontFace measureTextExtent: contents.
    self extent: textExtent
].

String ![
asMorph
    ^ StringMorph new contents: self; yourself
]