BorderedMorph subclass: #MenuBarMorph instanceVariables: #(elements).
Morph subclass: #MenuItemButtonMorph instanceVariables: #(label submenu actionBlock).
BorderedMorph subclass: #MenuMorph instanceVariables: #(elements).

MenuBarMorph class ![
defaultColor
    ^ Color lightGray.
].

MenuBarMorph class ![
defaultExtent
    ^ 100@30.
].

MenuItemButtonMorph class ![
defaultColor
    ^ Color lightGray.
].

MenuItemButtonMorph class ![
defaultExtent
    ^ 50@30.
].

MenuBarMorph ![
initialize
    super initialize.
    elements := ArrayList new.

    self layout: HorizontalMorphPackingLayout new.
].

MenuBarMorph ![
addItem: label submenu: submenu
    | button |
    button := MenuItemButtonMorph new label: label; submenu: submenu; yourself.
    elements add: button.
    self addMorph: button.

    self layout addMorph: button proportion: 0 expanded: true.
    self updateLayout
].

MenuMorph ![
initialize
    super initialize.
    elements := ArrayList new.

    self layout: VerticalMorphPackingLayout new.
].

MenuMorph ![
addItem: label withAction: aBlock
    | button |
    button := MenuItemButtonMorph new label: label; actionBlock: aBlock; yourself.
    elements add: button.
    self addMorph: button.

    self layout addMorph: button proportion: 0 expanded: true.
    self extent: 200 @ (elements size * 30).
    self updateLayout
].

MenuMorph ![
addSeparator
].

MenuItemButtonMorph ![
initialize
    super initialize.
].

MenuItemButtonMorph ![
renderWithBuilder: renderingBuilder
    self hasMouseFocus ifTrue: [
        renderingBuilder solidRectangle: self localBounds color: Color lightGray
    ].
    renderingBuilder text: label rectangle: self localBounds color: Color black.
].

MenuItemButtonMorph ![
handleMorphMouseMotionEvent: anEvent
    self grabMouseFocus.
    anEvent wasHandled: true.
    self changed
].

MenuItemButtonMorph ![
handleMorphMouseButtonDownEvent: anEvent
    self changed.
].

MorphEventVisitor ![
handleMorphMouseLeaveEvent: anEvent
    self changed
].

MenuItemButtonMorph ![
label
    ^ label
].

MenuItemButtonMorph ![
label: aString
    label := aString.
    self extent: (Font defaultFontFace measureTextExtent: label) + (10 @ 10)
].

MenuItemButtonMorph ![
submenu
    ^ submenu
].

MenuItemButtonMorph ![
submenu: aMenu
    submenu := aMenu
].

MenuItemButtonMorph ![
actionBlock
    ^ actionBlock
].

MenuItemButtonMorph ![
actionBlock: aBlockClosure
    actionBlock := aBlockClosure
].