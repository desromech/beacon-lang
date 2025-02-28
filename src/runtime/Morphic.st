Object subclass: #MorphLayout instanceVariables: #().

MorphLayout ![
applyWithExtent: anExtent
    self subclassResponsibility
].

MorphEventVisitor subclass: #Morph instanceVariables: #(position extent owner submorphs color visible layout).

Morph class ![
defaultColor
    ^ Color blue.
].

Morph class ![
defaultExtent
    ^ 50@40.
].

Morph ![
initialize
    super initialize.
    submorphs := #().
    position := 0@0.
    extent := self class defaultExtent.
    color := self class defaultColor.
    visible := true.
    layout := nil.
].

Morph ![
color
    ^ color
].

Morph ![
color: aColor
    color := aColor
].

Morph ![
bounds: aRectangle
    self position: aRectangle origin.
    self extent: aRectangle extent.
].

Morph ![
bounds
    ^ position extent: extent 
].

Morph ![
bounds: aRectangle
    self position: aRectangle origin.
    self extent: aRectangle extent.
].

Morph ![
childChanged: aChild
    owner ~~ nil ifTrue: [
        owner childChanged: aChild
    ].
].

Morph ![
changed
    owner ~~ nil ifTrue: [
        owner childChanged: self
    ].
].

Morph ![
localBounds
    ^ 0@0 corner: extent
].

Morph ![
layout
    ^ layout
].

Morph ![
layout: aLayout
    layout := aLayout.
    self updateLayout
].

Morph ![
extent
    ^ extent
].

Morph ![
extent: newExtent
    extent := newExtent.
    self updateLayout
].

Morph ![
width
    ^ extent x
].

Morph ![
height
    ^ extent y
].

Morph ![
localCenter
    ^ extent // (2@2)
].

Morph ![
position
    ^ position
].

Morph ![
position: newPosition
    position := newPosition
].

Morph ![
handleEvent: anEvent
    ^ anEvent accept: self
].

Morph ![
updateLayout
    layout == nil ifTrue: [ ^ self ].
    layout applyWithExtent: extent
].

Morph ![
processMouseEvent: anEvent
    anEvent translatedBy: self position negated during: [
        1 to: submorphs size do: [:i |
            | submorph |
            submorph := submorphs at: i.
            (submorph bounds containsPoint: anEvent position) ifTrue:[
                submorph processEvent: anEvent
            ]
        ].

        anEvent wasHandled ifFalse: [
            self handleEvent: anEvent
        ]
    ].
].

Morph ![
processEvent: anEvent
    anEvent isMouseEvent ifTrue: [
        ^ self processMouseEvent: anEvent
    ].

    self handleEvent: anEvent
].

Morph ![
renderWithBuilder: renderingBuilder
    renderingBuilder solidRectangle: self localBounds color: color
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

Morph ![
rootMorph
    ^ owner rootMorph
].

Morph subclass: #RootMorph instanceVariables: #(keyboardFocus mouseFocus ownerWindow).

RootMorph class ![
defaultColor
    ^ Color transparent
].

RootMorph ![
extent: newExtent
    | submorph |
    super extent: newExtent.
    submorphs size > 0 ifTrue: [
        (submorphs at: 1) extent: extent
    ]
].

RootMorph ![
rootMorph
    ^ self
].

RootMorph ![
ownerWindow
    ^ ownerWindow
].

RootMorph ![
ownerWindow: aWindow
    ownerWindow := aWindow
].

RootMorph ![
childChanged: childChanged
    ownerWindow ~~ nil ifTrue: [
        ownerWindow childChanged: childChanged
    ].
].

RootMorph ![
setMouseFocus: newMouseFocus
    mouseFocus == newMouseFocus ifTrue: [ ^ self ].
    mouseFocus ~~ nil ifTrue: [
        mouseFocus handleEvent: MorphMouseLeaveEvent new.
    ].

    mouseFocus := newMouseFocus.

    mouseFocus ~~ nil ifTrue: [
        mouseFocus handleEvent: MorphMouseEnterEvent new.
    ].

    ^ self
].

RootMorph ![
setKeyboardFocus: newKeyboardFocus
    keyboardFocus == newKeyboardFocus ifTrue: [ ^ self ].
    keyboardFocus ~~ nil ifTrue: [
        keyboardFocus handleEvent: MorphLostFocusEvent new.
    ].

    keyboardFocus := newKeyboardFocus.

    keyboardFocus ~~ nil ifTrue: [
        keyboardFocus handleEvent: MorphGotFocusEvent new.
    ].

    ^ self
].


RootMorph ![
processEvent: anEvent
    (anEvent isKeyboardEvent or: [anEvent isTextInputEvent]) ifTrue: [
        keyboardFocus == nil ifTrue: [^self].
        ^ keyboardFocus processEvent: anEvent
    ].

    super processEvent: anEvent
].

RootMorph ![
getMouseFocus
    ^ mouseFocus
].

Morph ![
hasMouseFocus
    ^ self rootMorph getMouseFocus == self
].

Morph ![
grabMouseFocus
    self rootMorph setMouseFocus: self
].

Morph ![
grabKeyboardFocus
    self rootMorph setKeyboardFocus: self
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
    renderingBuilder solidRectangle: self localBounds color: self color borderSize: self borderWidth borderColor: self borderColor
].

BorderedMorph ![
borderColor
    ^ borderColor
].

BorderedMorph ![
borderColor: aColor
    borderColor := aColor
].

BorderedMorph ![
borderWidth
    ^ borderWidth
].

BorderedMorph ![
borderWidth: aSize
    borderWidth := aSize
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
        solidRectangle: self bounds color: backgroundColor;
        text: contents rectangle: self bounds color: color
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

BorderedMorph subclass: #SimpleButtonMorph instanceVariables: #(label focusColor hasButtonDown clickColor onClickAction).

SimpleButtonMorph class ![
newWithLabel: labelString
    ^ self new
        label: labelString;
        yourself 
].

SimpleButtonMorph ![
initialize
    super initialize.
    color := Color r: 0.7 g: 0.7 b: 0.7 a: 1.0.
    focusColor := Color r: 0.8 g: 0.8 b: 0.8 a: 1.0.
    clickColor := Color r: 0.9 g: 0.9 b: 0.9 a: 1.0.
    hasButtonDown := false.
    borderWidth := 1.
    onClickAction := [].
    self setDefaultLabel
].

SimpleButtonMorph ![
color
    hasButtonDown ifTrue: [ ^ clickColor ].
    self hasMouseFocus ifTrue: [ ^ focusColor ].
    ^ color
].

SimpleButtonMorph ![
defaultLabel
    ^ 'Flash'
].

SimpleButtonMorph ![
onClickAction
    ^ onClickAction
].

SimpleButtonMorph ![
onClickAction: aBlock
    onClickAction := aBlock
].

SimpleButtonMorph ![
handleMorphMouseMotionEvent: anEvent
    self grabMouseFocus.
    anEvent wasHandled: true.
    self changed
].

SimpleButtonMorph ![
handleMorphMouseButtonDownEvent: anEvent
    hasButtonDown := true.
    anEvent wasHandled: true.
    self changed.
].

SimpleButtonMorph ![
handleMorphMouseButtonUpEvent: anEvent
    hasButtonDown := false.
    anEvent wasHandled: true.
    self changed.
    onClickAction value.
].

SimpleButtonMorph ![
handleMorphMouseEnterEvent: anEvent
    self changed.
].

SimpleButtonMorph ![
handleMorphMouseLeaveEvent: anEvent
    self changed.
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
