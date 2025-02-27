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
width
    ^ bounds width
].

Morph ![
height
    ^ bounds height
].

Morph ![
extent: newExtent
    self bounds: (bounds origin extent: newExtent)
].

Morph ![
localCenter
    ^ bounds halfExtent
].

Morph ![
center
    ^ bounds center
].

Morph ![
position
    ^ bounds origin
].

Morph ![
position: newPosition
    bounds := newPosition extent: bounds extent.
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

    renderingBuilder translatedBy: self position during: [
        self renderWithBuilder: renderingBuilder.
        self renderChildrenWithBuilder: renderingBuilder.
    ]
].

Morph ![
owner: aMorph
    owner := aMorph
].

Morph ![
addMorph: aMorph
    submorphs := submorphs copyWith: aMorph.
    aMorph owner: self
].

Morph ![
removeSubmorph: aMorph
    submorphs := submorphs copyWithout: aMorph.
    aMorph owner: nil
].

Morph ![
delete
    owner ~~ nil ifTrue: [
        owner removeSubmorph: self.
    ]
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
].

BorderedMorph subclass: #SimpleButtonMorph instanceVariables: #(label).

SimpleButtonMorph class ![
newWithLabel: labelString
    ^ self new
        label: labelString;
        yourself 
].

SimpleButtonMorph ![
initialize
    super initialize.
    color := Color r: 0.4 g: 0.8 b: 0.6 a: 1.0.
    borderWidth := 1.
    self setDefaultLabel
].

SimpleButtonMorph ![
defaultLabel
    ^ 'Flash'
].

SimpleButtonMorph ![
setDefaultLabel
    self label: self defaultLabel
].

SimpleButtonMorph ![
label: aString
    self label: aString font: Font defaultFontFace
].

SimpleButtonMorph ![
label: aLabelString font: aFontFace
    label ~~ nil ifTrue: [label delete].
    label := StringMorph new contents: aLabelString; yourself.
    self extent: (label width + 16) @ (label height + 16).
    label position: self localCenter - (label extent//(2@2)).
    self addMorph: label
].

SimpleButtonMorph ![
extent: newExtent
    super extent: newExtent.

    label ~~ nil ifTrue: [
        label position: self localCenter - (label extent//(2@2)).
    ].
].
